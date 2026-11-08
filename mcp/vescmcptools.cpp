/*
    Copyright 2026 Marvin Holzwarth

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
    */

#include "vescmcptools.h"

#include <QJsonDocument>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include "mcpparamvalidator.h"
#include "vescmcpdevice.h"
#include "mcpdatahelpers.h"
#include <QRegularExpression>
#include <QFile>
#include <QFileInfo>
#include <cmath>

namespace
{
bool validScriptName(const QString &name)
{
    return QRegularExpression("^[A-Za-z0-9_-]{1,64}$").match(name).hasMatch() &&
           !QRegularExpression("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$",
                               QRegularExpression::CaseInsensitiveOption)
                .match(name)
                .hasMatch();
}
QJsonObject tool(const QString &name, const QString &description, const QJsonObject &schema,
                 bool readOnly = true)
{
    return {{"name", name},
            {"description", description},
            {"inputSchema", schema},
            {"annotations", QJsonObject{{"readOnlyHint", readOnly},
                                        {"destructiveHint", !readOnly},
                                        {"idempotentHint", readOnly},
                                        {"openWorldHint", false}}}};
}
} // namespace

VescMcpTools::VescMcpTools(VescInterface *vesc, QObject *parent)
    : QObject(parent), mVesc(vesc), mSendCan(false), mCanId(-1)
{
    auto device = new VescMcpDevice(vesc);
    const auto appData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    mOperations = new McpDeviceOperations(
        device, appData.isEmpty() ? QString() : QDir(appData).filePath("mcp/controller-backups"),
        this);
    device->setParent(mOperations);
    connect(mVesc, &VescInterface::portConnectedChanged, this, &VescMcpTools::invalidateValues);
    connect(mVesc, &VescInterface::fwRxChanged, this, [this](bool, bool) { invalidateValues(); });
    connect(mVesc->commands(), &Commands::valuesReceived, this,
            [this](MC_VALUES values, unsigned int mask) {
                if (!mVesc->isPortConnected() || !mVesc->fwRx()) {
                    return;
                }
                // Only fields present in this packet are exposed. No zero-filled
                // selective fields or values from a previous CAN target are merged.
                mValues = McpDataHelpers::rtValues(values, mask);
                mSendCan = mVesc->commands()->getSendCan();
                mCanId = mVesc->commands()->getCanSendId();
                mValuesAge.start();
            });
}

QJsonObject VescMcpTools::configValues(ConfigParams *params) const
{
    QJsonObject values;
    for (const auto &key : params->getParamOrder()) {
        const auto param = params->getParamCopy(key);
        switch (param.type) {
        case CFG_T_DOUBLE:
            if (!std::isfinite(param.valDouble)) {
                return {};
            }
            values.insert(key, param.valDouble);
            break;
        case CFG_T_BOOL:
            values.insert(key, bool(param.valInt));
            break;
        case CFG_T_ENUM:
        case CFG_T_INT:
        case CFG_T_BITFIELD:
            values.insert(key, param.valInt);
            break;
        case CFG_T_QSTRING:
            values.insert(key, param.valString);
            break;
        default:
            break;
        }
    }
    return values;
}

QString VescMcpTools::snapshot(ConfigParams *params, const QString &config) const
{
    const auto values = configValues(params);
    if (values.isEmpty()) {
        return {};
    }
    const QJsonObject data{
        {"config", config}, {"values", values}, {"signature", double(params->getSignature())}};
    return QString::fromLatin1(
        QCryptographicHash::hash(QJsonDocument(data).toJson(QJsonDocument::Compact),
                                 QCryptographicHash::Sha256)
            .toHex());
}

QString VescMcpTools::saveBackup(ConfigParams *params, const QString &config, QString *error) const
{
    const auto hash = snapshot(params, config);
    if (hash.isEmpty()) {
        *error = "Configuration is empty or contains non-finite values.";
        return {};
    }
    const auto appDirectory =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (appDirectory.isEmpty()) {
        *error = "No local app data directory is available.";
        return {};
    }
    const auto directory = QDir(appDirectory).filePath("mcp/backups");
    if (!QDir().mkpath(directory)) {
        *error = "Could not create the local backup directory.";
        return {};
    }
    const auto prefix = directory + "/" +
                        QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz") + "-" +
                        QUuid::createUuid().toString(QUuid::WithoutBraces) + "-" + config;
    QByteArray xml;
    QXmlStreamWriter writer(&xml);
    writer.setAutoFormatting(true);
    params->getXML(writer, config == "motor" ? "MCConfiguration" : "APPConfiguration");
    if (writer.hasError()) {
        *error = "Could not serialize the XML backup.";
        return {};
    }
    const QJsonObject data{{"config", config},
                           {"source", "app_memory"},
                           {"snapshot", hash},
                           {"values", configValues(params)}};
    auto write = [error](const QString &path, const QByteArray &bytes) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            *error = "Could not open the local backup.";
            return false;
        }
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        if (file.write(bytes) != bytes.size() || !file.commit()) {
            *error = "Could not commit the local backup.";
            return false;
        }
        return true;
    };
    // Use the existing XML format for GUI import, plus JSON for full double
    // precision: the project's XML serializer prints doubles with six digits.
    if (!write(prefix + ".xml", xml) ||
        !write(prefix + ".json", QJsonDocument(data).toJson(QJsonDocument::Indented))) {
        return {};
    }
    return prefix + ".json";
}

QJsonObject VescMcpTools::editConfig(const QJsonObject &arguments, bool apply)
{
    if (!mAllowEdits) {
        return failure("Offline app edits are disabled. Restart with --mcpAllowEdits.");
    }
    if (arguments.size() != 3 || !arguments.contains("config") || !arguments.contains("changes") ||
        !arguments.contains("expectedSnapshot")) {
        return failure("config, changes and expectedSnapshot are required.");
    }
    const auto config = arguments.value("config").toString();
    if (config != "motor" && config != "app") {
        return failure("config must be 'motor' or 'app'.");
    }
    if (!arguments.value("changes").isObject() || !arguments.value("expectedSnapshot").isString()) {
        return failure("Invalid changes or snapshot type.");
    }
    auto params = config == "motor" ? mVesc->mcConfig() : mVesc->appConfig();
    const auto hash = snapshot(params, config);
    if (hash.isEmpty() || arguments.value("expectedSnapshot").toString() != hash) {
        return failure("Configuration changed. Read a new snapshot and preview again.");
    }
    const auto changes = arguments.value("changes").toObject();
    if (changes.isEmpty() || changes.size() > 64) {
        return failure("Provide between 1 and 64 parameter changes.");
    }
    if (mOperations->busy()) {
        return failure("Finish the current controller operation first.");
    }
    if (apply && (!params->getUpdatesEnabled() || !params->getUpdateOnly().isEmpty())) {
        return failure("Finish the configuration update before applying app-memory edits.");
    }
    const auto before = configValues(params);
    QJsonArray diff;
    for (auto it = changes.begin(); it != changes.end(); ++it) {
        if (!params->hasParam(it.key())) {
            return failure("Unknown parameter: " + it.key());
        }
        QString error;
        if (!McpParamValidator::validate(params->getParamCopy(it.key()), it.value(), &error)) {
            return failure(it.key() + ": " + error);
        }
        diff.append(QJsonObject{
            {"name", it.key()}, {"before", before.value(it.key())}, {"after", it.value()}});
    }
    if (!apply) {
        return result({{"config", config},
                       {"snapshot", hash},
                       {"diff", diff},
                       {"applied", false},
                       {"controllerWritten", false}});
    }
    QString error;
    const auto backup = saveBackup(params, config, &error);
    if (backup.isEmpty()) {
        return failure(error);
    }
    // Every requested value has been checked before any edit. Use the usual
    // ConfigParams setters so the GUI gets its existing change signals.
    for (auto it = changes.begin(); it != changes.end(); ++it) {
        switch (params->getParamCopy(it.key()).type) {
        case CFG_T_DOUBLE:
            params->updateParamDouble(it.key(), it.value().toDouble(), this);
            break;
        case CFG_T_INT:
            params->updateParamInt(it.key(), it.value().toInt(), this);
            break;
        case CFG_T_ENUM:
            params->updateParamEnum(it.key(), it.value().toInt(), this);
            break;
        case CFG_T_BOOL:
            params->updateParamBool(it.key(), it.value().toBool(), this);
            break;
        case CFG_T_QSTRING:
            params->updateParamString(it.key(), it.value().toString(), this);
            break;
        default:
            break;
        }
    }
    const auto actual = configValues(params);
    bool verified = true;
    for (auto it = changes.begin(); it != changes.end(); ++it) {
        verified &= actual.value(it.key()) == it.value();
    }
    return result({{"config", config},
                   {"snapshot", snapshot(params, config)},
                   {"diff", diff},
                   {"applied", true},
                   {"verified", verified},
                   {"backupJsonPath", backup},
                   {"controllerWritten", false}},
                  !verified);
}

QJsonObject VescMcpTools::deviceTool(const QString &name, const QJsonObject &arguments)
{
    if (name == "vesc_get_job" || name == "vesc_cancel_job") {
        for (auto it = arguments.begin(); it != arguments.end(); ++it) {
            if (it.key() != "id" && !(name == "vesc_get_job" && it.key() == "includeSamples")) {
                return failure("Unknown argument.");
            }
        }
        if (!arguments.value("id").isString() ||
            (arguments.contains("includeSamples") && !arguments.value("includeSamples").isBool())) {
            return failure("A job id and optional boolean includeSamples are required.");
        }
        const auto id = arguments.value("id").toString();
        if (name == "vesc_cancel_job") {
            mOperations->cancel(id);
        }
        const auto data = mOperations->job(id, arguments.value("includeSamples").toBool());
        return result(data, !data.value("error").toString().isEmpty() && !data.contains("state"));
    }
    if (name == "vesc_controller_download" || name == "vesc_controller_upload") {
        if (arguments.size() != 2 || !arguments.contains("config") ||
            !arguments.contains("expectedSnapshot")) {
            return failure("config and expectedSnapshot are required.");
        }
        const auto kind = arguments.value("config").toString();
        if (kind != "motor" && kind != "app") {
            return failure("Invalid config kind.");
        }
        if (arguments.value("expectedSnapshot").toString() !=
            snapshot(kind == "motor" ? mVesc->mcConfig() : mVesc->appConfig(), kind)) {
            return failure("App configuration changed. Read a current snapshot first.");
        }
        if (name == "vesc_controller_upload" && !mAllowControllerWrites) {
            return failure("Controller writes are disabled.");
        }
        const auto data =
            mOperations->start(name == "vesc_controller_upload" ? "upload" : "download", arguments);
        return result(data, !data.value("error").toString().isEmpty());
    }
    if (name == "vesc_rt_capture") {
        QJsonObject args;
        for (auto it = arguments.begin(); it != arguments.end(); ++it) {
            if (it.key() != "durationMs" && it.key() != "intervalMs") {
                return failure("Unknown argument.");
            }
            const double v = it.value().toDouble(-1);
            const int min = it.key() == "durationMs" ? 100 : 50;
            const int max = it.key() == "durationMs" ? 60000 : 1000;
            if (!it.value().isDouble() || !std::isfinite(v) || std::floor(v) != v || v < min ||
                v > max) {
                return failure("Invalid capture duration or interval.");
            }
            args.insert(it.key(), int(v));
        }
        const auto data = mOperations->start("capture", args);
        return result(data, !data.value("error").toString().isEmpty());
    }
    if (name == "vesc_lisp_download") {
        if (!arguments.isEmpty()) {
            return failure("This tool takes no arguments.");
        }
        const auto data = mOperations->start("lisp_download", {});
        return result(data, !data.value("error").toString().isEmpty());
    }
    if (!mAllowControllerWrites) {
        return failure("Controller writes are disabled.");
    }
    if (arguments.size() != 2 || !arguments.value("name").isString() ||
        !validScriptName(arguments.value("name").toString())) {
        return failure("A draft name and expectedSha256 are required.");
    }
    const auto root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (root.isEmpty()) {
        return failure("No local app data directory.");
    }
    QFile file(QDir(root).filePath("mcp/scripts/" + arguments.value("name").toString() + ".lbm"));
    if (QFileInfo(file).isSymLink()) {
        return failure("Symbolic-link drafts are not supported.");
    }
    if (!file.open(QIODevice::ReadOnly) || file.size() > 32768) {
        return failure("Could not read the named LispBM draft.");
    }
    const auto bytes = file.readAll();
    const auto hash =
        QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
    if (arguments.value("expectedSha256").toString() != hash) {
        return failure("The draft changed; read its current SHA-256.");
    }
    const auto data = mOperations->start("lisp_upload", {{"source", QString::fromUtf8(bytes)}});
    return result(data, !data.value("error").toString().isEmpty());
}

QJsonObject VescMcpTools::scriptTool(const QString &name, const QJsonObject &arguments)
{
    const bool write = name == "vesc_lisp_save";
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
        if (it.key() != "name" &&
            !(write && (it.key() == "source" || it.key() == "expectedSha256"))) {
            return failure("Unknown argument.");
        }
    }
    if (!arguments.value("name").isString() ||
        !validScriptName(arguments.value("name").toString())) {
        return failure("Use a draft name with 1-64 letters, digits, '_' or '-'.");
    }
    const auto root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (root.isEmpty()) {
        return failure("No local app data directory.");
    }
    const auto directory = QDir(root).filePath("mcp/scripts");
    const auto path = QDir(directory).filePath(arguments.value("name").toString() + ".lbm");
    QByteArray bytes;
    QFile existing(path);
    if (QFileInfo(existing).isSymLink()) {
        return failure("Symbolic-link drafts are not supported.");
    }
    const bool exists = existing.exists();
    if (exists) {
        if (!existing.open(QIODevice::ReadOnly) || existing.size() > 32768) {
            return failure("Could not read draft.");
        }
        bytes = existing.readAll();
        existing.close();
    }
    if (write) {
        if (!mAllowEdits || !arguments.value("source").isString()) {
            return failure("Local edits are disabled or source is missing.");
        }
        const auto current = QString::fromLatin1(
            QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        if ((exists && arguments.value("expectedSha256").toString() != current) ||
            (!exists && arguments.contains("expectedSha256"))) {
            return failure("Draft conflict; read the existing draft first.");
        }
        QString error;
        if (McpDataHelpers::lispImage(arguments.value("source").toString(), &error).isEmpty()) {
            return failure(error);
        }
        bytes = arguments.value("source").toString().toUtf8();
        if (!QDir().mkpath(directory)) {
            return failure("Could not create the scripts directory.");
        }
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return failure("Could not open draft.");
        }
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        if (file.write(bytes) != bytes.size() || !file.commit()) {
            return failure("Could not save draft.");
        }
    } else if (!exists) {
        return failure("Unknown draft.");
    }
    return result(
        {{"name", arguments.value("name")},
         {"path", path},
         {"source", QString::fromUtf8(bytes)},
         {"sha256", QString::fromLatin1(
                        QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())}});
}

void VescMcpTools::invalidateValues()
{
    mValuesAge.invalidate();
    mValues = {};
}

QJsonArray VescMcpTools::tools() const
{
    const QJsonObject emptySchema{
        {"type", "object"}, {"properties", QJsonObject()}, {"additionalProperties", false}};
    const QJsonObject configSchema{
        {"type", "object"},
        {"properties",
         QJsonObject{
             {"config", QJsonObject{{"type", "string"}, {"enum", QJsonArray{"motor", "app"}}}},
             {"names", QJsonObject{{"type", "array"},
                                   {"maxItems", 64},
                                   {"items", QJsonObject{{"type", "string"}}}}}}},
        {"required", QJsonArray{"config"}},
        {"additionalProperties", false}};
    QJsonArray list{
        tool("vesc_get_status", "Read connection state of this VESC Tool app.", emptySchema),
        tool("vesc_get_config",
             "Read motor or app parameters from app memory, including unsaved edits. "
             "Does not read from the controller or prove persisted controller settings.",
             configSchema),
        tool("vesc_get_rt_data",
             "Read the last values packet already received by the app. "
             "Returns age and validity. Does not enable polling or request controller data.",
             emptySchema)};
    if (mAllowEdits) {
        const QJsonObject editSchema{
            {"type", "object"},
            {"properties",
             QJsonObject{
                 {"config", QJsonObject{{"type", "string"}, {"enum", QJsonArray{"motor", "app"}}}},
                 {"changes",
                  QJsonObject{{"type", "object"}, {"minProperties", 1}, {"maxProperties", 64}}},
                 {"expectedSnapshot",
                  QJsonObject{{"type", "string"}, {"pattern", "^[a-f0-9]{64}$"}}}}},
            {"required", QJsonArray{"config", "changes", "expectedSnapshot"}},
            {"additionalProperties", false}};
        list.append(tool("vesc_preview_config_edit",
                         "Validate and preview offline app-memory edits. No changes are made.",
                         editSchema));
        list.append(tool("vesc_apply_config_edit",
                         "Back up and apply validated edits to offline app memory only. Never "
                         "writes the controller.",
                         editSchema, false));
        list.append(
            tool("vesc_save_config_local",
                 "Save app-memory XML and exact JSON backup in the app data directory. Never "
                 "writes the controller.",
                 QJsonObject{
                     {"type", "object"},
                     {"properties",
                      QJsonObject{{"config", QJsonObject{{"type", "string"},
                                                         {"enum", QJsonArray{"motor", "app"}}}}}},
                     {"required", QJsonArray{"config"}},
                     {"additionalProperties", false}},
                 false));
    }
    const QJsonObject kind{{"type", "string"}, {"enum", QJsonArray{"motor", "app"}}};
    const QJsonObject hash{{"type", "string"}, {"pattern", "^[a-f0-9]{64}$"}};
    auto schema = [](const QJsonObject &properties, const QJsonArray &required) {
        return QJsonObject{{"type", "object"},
                           {"properties", properties},
                           {"required", required},
                           {"additionalProperties", false}};
    };
    const auto transfer =
        schema({{"config", kind}, {"expectedSnapshot", hash}}, {"config", "expectedSnapshot"});
    list.append(tool(
        "vesc_controller_download",
        "Download fresh controller config into this app and save local backups. Returns a job id.",
        transfer, false));
    list.append(tool("vesc_get_job",
                     "Read controller job progress/results. RT samples are optional.",
                     schema({{"id", QJsonObject{{"type", "string"}}},
                             {"includeSamples", QJsonObject{{"type", "boolean"}}}},
                            {"id"})));
    list.append(tool("vesc_cancel_job",
                     "Cancel a pending job. An issued write cannot be presumed undone.",
                     schema({{"id", QJsonObject{{"type", "string"}}}}, {"id"}), false));
    list.append(tool(
        "vesc_rt_capture",
        "Poll and analyze a bounded RT recording through the existing app connection. No motor "
        "commands.",
        schema(
            {{"durationMs", QJsonObject{{"type", "integer"}, {"minimum", 100}, {"maximum", 60000}}},
             {"intervalMs", QJsonObject{{"type", "integer"}, {"minimum", 50}, {"maximum", 1000}}}},
            {}),
        false));
    list.append(tool("vesc_lisp_download",
                     "Download and locally back up existing LispBM code. Returns a job id.",
                     emptySchema, false));
    const auto name = QJsonObject{{"type", "string"}, {"pattern", "^[A-Za-z0-9_-]{1,64}$"}};
    list.append(tool("vesc_lisp_get", "Read a named local LispBM draft and its SHA-256.",
                     schema({{"name", name}}, {"name"})));
    if (mAllowEdits) {
        list.append(tool("vesc_lisp_save",
                         "Create or update a local self-contained LispBM draft. Existing drafts "
                         "require their current SHA-256.",
                         schema({{"name", name},
                                 {"source", QJsonObject{{"type", "string"}}},
                                 {"expectedSha256", hash}},
                                {"name", "source"}),
                         false));
    }
    if (mAllowControllerWrites) {
        list.append(tool("vesc_controller_upload",
                         "Upload the current app config after backup and manual approval in VESC "
                         "Tool, then verify controller readback. Returns a job id.",
                         transfer, false));
        list.append(tool(
            "vesc_lisp_upload",
            "Upload a saved LispBM draft after manual approval and backup. Stops the VM; does not "
            "start the script. Verifies readback.",
            schema({{"name", name}, {"expectedSha256", hash}}, {"name", "expectedSha256"}), false));
    }
    return list;
}

void VescMcpTools::setAllowEdits(bool allow)
{
    mAllowEdits = allow;
}

void VescMcpTools::setAllowControllerWrites(bool allow)
{
    mAllowControllerWrites = allow;
}

QJsonObject VescMcpTools::result(const QJsonObject &data, bool error)
{
    return {{"content",
             QJsonArray{QJsonObject{
                 {"type", "text"},
                 {"text", QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact))}}}},
            {"isError", error}};
}

QJsonObject VescMcpTools::failure(const QString &message)
{
    return result({{"error", message}}, true);
}

QJsonObject VescMcpTools::call(const QString &name, const QJsonObject &arguments)
{
    if (name == "vesc_controller_download" || name == "vesc_controller_upload" ||
        name == "vesc_get_job" || name == "vesc_cancel_job" || name == "vesc_rt_capture" ||
        name == "vesc_lisp_download" || name == "vesc_lisp_upload") {
        return deviceTool(name, arguments);
    }
    if (name == "vesc_lisp_save" || name == "vesc_lisp_get") {
        return scriptTool(name, arguments);
    }
    if (name == "vesc_preview_config_edit" || name == "vesc_apply_config_edit") {
        return editConfig(arguments, name == "vesc_apply_config_edit");
    }
    if (name == "vesc_save_config_local") {
        if (!mAllowEdits || arguments.size() != 1) {
            return failure("Local writes are disabled or arguments are invalid.");
        }
        const auto config = arguments.value("config").toString();
        if (config != "motor" && config != "app") {
            return failure("config must be 'motor' or 'app'.");
        }
        auto params = config == "motor" ? mVesc->mcConfig() : mVesc->appConfig();
        QString error;
        const auto path = saveBackup(params, config, &error);
        return path.isEmpty() ? failure(error)
                              : result({{"jsonPath", path},
                                        {"xmlPath", path.left(path.size() - 5) + ".xml"},
                                        {"source", "app_memory"},
                                        {"controllerWritten", false}});
    }
    if (name == "vesc_get_status") {
        if (!arguments.isEmpty()) {
            return failure("This tool takes no arguments.");
        }
        const auto fw = mVesc->getLastFwRxParams();
        const bool firmwareValid = mVesc->isPortConnected() && mVesc->fwRx();
        return result(
            {{"connected", mVesc->isPortConnected()},
             {"firmwareReceived", mVesc->fwRx()},
             {"firmwareVersion", firmwareValid ? QJsonValue(QString("%1.%2").arg(fw.major).arg(
                                                     fw.minor, 2, 10, QLatin1Char('0')))
                                               : QJsonValue()},
             {"hardware", firmwareValid ? QJsonValue(fw.hw) : QJsonValue()},
             {"configurationSchemaCompatible",
              firmwareValid && fw.hwType == HW_TYPE_VESC &&
                  mVesc->getSupportedFirmwarePairs().contains(qMakePair(fw.major, fw.minor))},
             {"limitedMode", mVesc->commands()->isLimitedMode()},
             {"canForwarding", mVesc->commands()->getSendCan()},
             {"canId", mVesc->commands()->getCanSendId()},
             {"access", mAllowEdits ? "app_edits" : "read_only"},
             {"controllerWritesEnabled", mAllowControllerWrites},
             {"controllerBusy", mOperations->busy()},
             {"source", "app_memory"}});
    }
    if (name == "vesc_get_rt_data") {
        if (!arguments.isEmpty()) {
            return failure("This tool takes no arguments.");
        }
        const bool valid = mValuesAge.isValid() && mVesc->isPortConnected() && mVesc->fwRx() &&
                           mSendCan == mVesc->commands()->getSendCan() &&
                           mCanId == mVesc->commands()->getCanSendId();
        return result({{"available", valid},
                       {"source", "app_cache"},
                       {"ageMs", valid ? QJsonValue(double(mValuesAge.elapsed())) : QJsonValue()},
                       {"stale", !valid || mValuesAge.elapsed() > 1000},
                       {"values", valid ? mValues : QJsonObject()}});
    }
    if (name != "vesc_get_config") {
        return failure("Unknown tool.");
    }
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
        if (it.key() != "config" && it.key() != "names") {
            return failure("Unknown argument.");
        }
    }
    const auto config = arguments.value("config").toString();
    if (config != "motor" && config != "app") {
        return failure("config must be 'motor' or 'app'.");
    }
    auto params = config == "motor" ? mVesc->mcConfig() : mVesc->appConfig();
    QStringList names = params->getParamOrder();
    if (arguments.contains("names")) {
        if (!arguments.value("names").isArray() || arguments.value("names").toArray().size() > 64) {
            return failure("names must be an array of at most 64 parameter names.");
        }
        names.clear();
        for (const auto value : arguments.value("names").toArray()) {
            if (!value.isString() || !params->hasParam(value.toString())) {
                return failure("Unknown parameter name.");
            }
            names.append(value.toString());
        }
    }
    QJsonObject values;
    for (const auto &key : names) {
        const auto param = params->getParamCopy(key);
        switch (param.type) {
        case CFG_T_DOUBLE:
            if (!std::isfinite(param.valDouble)) {
                return failure("Configuration contains a non-finite value.");
            }
            values.insert(key, param.valDouble);
            break;
        case CFG_T_INT:
        case CFG_T_ENUM:
        case CFG_T_BITFIELD:
            values.insert(key, param.valInt);
            break;
        case CFG_T_BOOL:
            values.insert(key, bool(param.valInt));
            break;
        case CFG_T_QSTRING:
            values.insert(key, param.valString);
            break;
        default:
            break;
        }
    }
    return result({{"config", config},
                   {"source", "app_memory"},
                   {"controllerValuesVerified", false},
                   {"snapshot", snapshot(params, config)},
                   {"values", values}});
}
