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

#include "mcpdatahelpers.h"
#include "vbytearray.h"
#include "packet.h"
#include "datatypes.h"
#include <QRegularExpression>
#include <QSet>
#include <cmath>
#include <limits>

QJsonObject McpDataHelpers::rtValues(const MC_VALUES &v, quint32 mask)
{
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
    field(4, "current_d_a", v.id);
    field(5, "current_q_a", v.iq);
    field(6, "duty", v.duty_now);
    field(7, "erpm", v.rpm);
    field(8, "voltage_v", v.v_in);
    field(9, "amp_hours_ah", v.amp_hours);
    field(10, "amp_hours_charged_ah", v.amp_hours_charged);
    field(11, "watt_hours_wh", v.watt_hours);
    field(12, "watt_hours_charged_wh", v.watt_hours_charged);
    field(13, "tachometer", v.tachometer);
    field(14, "tachometer_abs", v.tachometer_abs);
    if (mask & (quint32(1) << 15)) {
        fields.insert("fault_code", int(v.fault_code));
        if (!v.fault_str.isEmpty()) {
            fields.insert("fault", v.fault_str);
        }
    }
    field(16, "pid_position_deg", v.position);
    field(17, "controller_id", v.vesc_id);
    field(18, "temp_mos_1_c", v.temp_mos_1);
    field(18, "temp_mos_2_c", v.temp_mos_2);
    field(18, "temp_mos_3_c", v.temp_mos_3);
    field(19, "voltage_d_v", v.vd);
    field(20, "voltage_q_v", v.vq);
    if (mask & (quint32(1) << 21)) {
        fields.insert("has_timeout", v.has_timeout);
        fields.insert("kill_switch_active", v.kill_sw_active);
    }
    return fields;
}

QJsonObject McpDataHelpers::analyze(const QJsonArray &samples)
{
    QSet<QString> keys;
    for (const auto &item : samples) {
        const auto sample = item.toObject();
        for (auto it = sample.begin(); it != sample.end(); ++it) {
            if (it.key() != "timeMs" && it.value().isDouble()) {
                keys.insert(it.key());
            }
        }
    }
    QJsonObject fields;
    int lowSpeedCurrent = 0;
    for (const auto &item : samples) {
        const auto s = item.toObject();
        if (s.value("erpm").isDouble() && s.value("current_motor_a").isDouble() &&
            std::isfinite(s.value("erpm").toDouble()) &&
            std::isfinite(s.value("current_motor_a").toDouble()) &&
            std::abs(s.value("erpm").toDouble()) < 30 &&
            std::abs(s.value("current_motor_a").toDouble()) > 2) {
            lowSpeedCurrent++;
        }
    }
    for (const auto &key : keys) {
        int count = 0;
        double mean = 0, m2 = 0, squares = 0;
        double min = std::numeric_limits<double>::infinity(), max = -min;
        for (const auto &item : samples) {
            const auto v = item.toObject().value(key);
            if (!v.isDouble() || !std::isfinite(v.toDouble())) {
                continue;
            }
            const double x = v.toDouble(), delta = x - mean;
            count++;
            mean += delta / count;
            m2 += delta * (x - mean);
            squares += x * x;
            min = qMin(min, x);
            max = qMax(max, x);
        }
        if (count) {
            fields.insert(key, QJsonObject{{"count", count},
                                           {"min", min},
                                           {"max", max},
                                           {"mean", mean},
                                           {"rms", std::sqrt(squares / count)},
                                           {"stddev", std::sqrt(qMax(0.0, m2 / count))}});
        }
    }
    return {{"samples", samples.size()},
            {"fields", fields},
            {"lowSpeedCurrentSamples", lowSpeedCurrent},
            {"interpretation",
             "Low-speed current is an observation, not a diagnosis of a stalled motor."}};
}

QByteArray McpDataHelpers::lispImage(const QString &source, QString *error)
{
    const auto bytes = source.toUtf8();
    if (bytes.isEmpty() || bytes.size() > 32768 || source.contains(QChar(0)) ||
        QRegularExpression("\\(\\s*import\\b").match(source).hasMatch()) {
        *error = "Use a nonempty, self-contained LispBM source of at most 32 KiB without NUL or "
                 "imports.";
        return {};
    }
    VByteArray packed;
    packed.vbAppendUint16(0);
    packed.append(bytes);
    packed.append('\0');
    packed.vbAppendInt16(0);
    VByteArray image;
    image.vbAppendUint32(packed.size() - 2);
    image.vbAppendUint16(
        Packet::crc16(reinterpret_cast<const unsigned char *>(packed.constData()), packed.size()));
    image.append(packed);
    return image;
}
QByteArray McpDataHelpers::lispReadback(const QByteArray &image)
{
    return image.mid(8);
}
QString McpDataHelpers::lispSource(const QByteArray &payload)
{
    const int end = payload.indexOf('\0');
    const auto bytes = end >= 0 ? payload.left(end) : payload;
    return QString::fromUtf8(bytes);
}
