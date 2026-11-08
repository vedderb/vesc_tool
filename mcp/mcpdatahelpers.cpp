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
#include <QRegularExpression>
#include <QSet>
#include <cmath>
#include <limits>

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
