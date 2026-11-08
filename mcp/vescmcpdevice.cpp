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

#include "vescmcpdevice.h"
#include "mcpparamvalidator.h"
#include <QXmlStreamWriter>
#include <QApplication>
#include <cmath>

VescMcpDevice::VescMcpDevice(VescInterface *vesc, QObject *parent) : McpDevice(parent), mVesc(vesc)
{
    auto commands = mVesc->commands();
    connect(commands, &Commands::exclusiveInterrupted, this, [this]() {
        emit fault("The app emergency-stop path interrupted the controller operation.");
    });
    connect(mVesc->mcConfig(), &ConfigParams::updated, this,
            [this]() { emit configuration("motor"); });
    connect(mVesc->appConfig(), &ConfigParams::updated, this,
            [this]() { emit configuration("app"); });
    connect(commands, &Commands::fwVersionReceived, this,
            [this](FW_RX_PARAMS) { emit firmware(); });
    connect(commands, &Commands::ackReceived, this, &McpDevice::acknowledgment);
    connect(commands, &Commands::deserializeConfigFailed, this, [this](bool, bool) {
        emit fault("Controller configuration signature/deserialization failed.");
    });
    connect(commands, &Commands::decodedAdcReceived, this,
            [this](double value, double, double value2, double) {
                emit input({{"type", "adc"}, {"value", value}, {"value2", value2}});
            });
    connect(commands, &Commands::decodedPpmReceived, this,
            [this](double value, double) { emit input({{"type", "ppm"}, {"value", value}}); });
    connect(commands, &Commands::valuesReceived, this, [this](MC_VALUES v, unsigned int mask) {
        QJsonObject fields;
        auto field = [&fields, mask](int bit, const QString &key, double value) {
            if ((mask & (quint32(1) << bit)) && std::isfinite(value)) {
                fields.insert(key, value);
            }
        };
        field(0, "temp_mos_c", v.temp_mos);
        field(1, "temp_motor_c", v.temp_motor);
        field(2, "current_motor_a", v.current_motor);
        field(3, "current_in_a", v.current_in);
        field(6, "duty", v.duty_now);
        field(7, "erpm", v.rpm);
        field(8, "voltage_v", v.v_in);
        if (mask & (quint32(1) << 15)) {
            fields.insert("fault_code", int(v.fault_code));
        }
        emit telemetry(fields);
    });
    connect(commands, &Commands::lispReadCodeRx, this, &McpDevice::lispCode);
    connect(commands, &Commands::lispEraseCodeRx, this, &McpDevice::lispErased);
    connect(commands, &Commands::lispWriteCodeRx, this, &McpDevice::lispWritten);
}

bool VescMcpDevice::ready() const
{
    if (!mVesc || !mVesc->isPortConnected() || !mVesc->fwRx() || mVesc->tcpServerIsRunning() ||
        mVesc->udpServerIsRunning() || target().isEmpty()) {
        return false;
    }
    const auto fw = mVesc->getLastFwRxParams();
    // Require the actually loaded parameter schema, not merely an available
    // archive. VESC Tool can load an older schema while retaining limited mode.
    return fw.hwType == HW_TYPE_VESC &&
           mVesc->getSupportedFirmwarePairs().contains(qMakePair(fw.major, fw.minor));
}

QString VescMcpDevice::target() const
{
    if (!mVesc) {
        return {};
    }
    const auto fw = mVesc->getLastFwRxParams();
    if (fw.uuid.isEmpty() || fw.uuid == QByteArray(fw.uuid.size(), '\0')) {
        return {};
    }
    return QString::fromLatin1(fw.uuid.toHex()) + "/" + QString::number(fw.major) + "." +
           QString::number(fw.minor) + "/" + QString::number(fw.hwConfCrc) + "/" +
           (mVesc->commands()->getSendCan() ? QString::number(mVesc->commands()->getCanSendId())
                                            : "local");
}

bool VescMcpDevice::acquire(QObject *owner)
{
    return !QApplication::activeModalWidget() && ready() &&
           mVesc->commands()->acquireExclusive(owner);
}
void VescMcpDevice::release(QObject *owner)
{
    if (mVesc) {
        mVesc->commands()->releaseExclusive(owner);
    }
}
bool VescMcpDevice::send(QObject *owner, const QByteArray &packet)
{
    return ready() && mVesc->commands()->sendExclusive(owner, packet);
}
ConfigParams *VescMcpDevice::params(const QString &kind) const
{
    return kind == "motor" ? mVesc->mcConfig() : mVesc->appConfig();
}
QJsonObject VescMcpDevice::values(ConfigParams *params)
{
    QJsonObject result;
    for (const auto &name : params->getSerializeOrder()) {
        const auto p = params->getParamCopy(name);
        switch (p.type) {
        case CFG_T_DOUBLE:
            result.insert(name, p.valDouble);
            break;
        case CFG_T_BOOL:
            result.insert(name, bool(p.valInt));
            break;
        case CFG_T_QSTRING:
            result.insert(name, p.valString);
            break;
        case CFG_T_INT:
        case CFG_T_ENUM:
        case CFG_T_BITFIELD:
            result.insert(name, p.valInt);
            break;
        default:
            break;
        }
    }
    return result;
}
QJsonObject VescMcpDevice::configValues(const QString &kind) const
{
    return values(params(kind));
}
QByteArray VescMcpDevice::configWire(const QString &kind) const
{
    VByteArray bytes;
    params(kind)->serialize(bytes);
    return bytes;
}
QByteArray VescMcpDevice::configXml(const QString &kind) const
{
    QByteArray bytes;
    QXmlStreamWriter stream(&bytes);
    stream.setAutoFormatting(true);
    params(kind)->getXML(stream, kind == "motor" ? "MCConfiguration" : "APPConfiguration");
    return stream.hasError() ? QByteArray() : bytes;
}
QJsonObject VescMcpDevice::canonicalValues(const QString &kind, QString *error) const
{
    ConfigParams copy;
    copy = *params(kind);
    copy.setUpdatesEnabled(true);
    copy.setUpdateOnly("");
    VByteArray bytes(configWire(kind));
    if (!copy.deSerialize(bytes)) {
        *error = "Could not normalize the proposed wire configuration.";
        return {};
    }
    return values(&copy);
}
bool VescMcpDevice::validateChanges(const QString &kind, const QJsonObject &desired,
                                    QString *error) const
{
    const auto before = configValues(kind);
    if (before.keys() != desired.keys()) {
        *error = "Parameter schema changed.";
        return false;
    }
    for (auto it = desired.begin(); it != desired.end(); ++it) {
        if (kind == "app" && it.key() == "controller_id" && before.value(it.key()) != it.value() &&
            mVesc->commands()->getSendCan()) {
            *error = "Changing the CAN id of a forwarded device prevents verified readback; "
                     "connect directly first.";
            return false;
        }
        if (kind == "app" && it.key() == "app_to_use" && before.value(it.key()) != it.value()) {
            const auto mode = params(kind)->getParamEnumNames(it.key()).value(it.value().toInt());
            if (mode != "No App" && mode != "ADC" && mode != "PPM") {
                *error = "The proposed active-input mode cannot be verified by this upload tool.";
                return false;
            }
        }
        if (before.value(it.key()) != it.value() &&
            !McpParamValidator::validate(params(kind)->getParamCopy(it.key()), it.value(), error)) {
            *error = it.key() + ": " + *error;
            return false;
        }
    }
    return true;
}
QString VescMcpDevice::inputMode() const
{
    auto p = mVesc->appConfig();
    const auto app = p->getParamEnumNames("app_to_use").value(p->getParamEnum("app_to_use"));
    if (app == "ADC") {
        return "adc";
    }
    if (app == "PPM") {
        return "ppm";
    }
    if (app == "No App") {
        return "none";
    }
    return "unsupported";
}
