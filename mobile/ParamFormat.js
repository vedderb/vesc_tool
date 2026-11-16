/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

.pragma library

// Formats the value of a parameter for display, e.g. "60.00 A" or "UART"
function valueString(conf, name) {
    if (conf === null || !conf.hasParam(name)) {
        return ""
    }

    if (conf.isParamDouble(name) || conf.isParamInt(name)) {
        var isDouble = conf.isParamDouble(name)
        var val = isDouble ? conf.getParamDouble(name) : conf.getParamInt(name)

        if (conf.getParamEditAsPercentage(name)) {
            var maxAbs = isDouble ?
                        Math.max(Math.abs(conf.getParamMaxDouble(name)), Math.abs(conf.getParamMinDouble(name))) :
                        Math.max(Math.abs(conf.getParamMaxInt(name)), Math.abs(conf.getParamMinInt(name)))
            return Math.round(100 * val / maxAbs) + " %"
        }

        return (val * conf.getParamEditorScale(name)).toFixed(
                    isDouble ? conf.getParamDecimalsDouble(name) : 0) + conf.getParamSuffix(name)
    }

    if (conf.isParamEnum(name)) {
        return conf.getParamEnumNames(name)[conf.getParamEnum(name)]
    }

    if (conf.isParamBool(name)) {
        return conf.getParamBool(name) ? qsTr("On") : qsTr("Off")
    }

    if (conf.isParamBitfield(name)) {
        var bits = conf.getParamInt(name)
        var selected = 0
        for (var i = 0;i < 8;i++) {
            selected += (bits >> i) & 1
        }
        return qsTr("%1 selected").arg(selected)
    }

    return conf.getParamQString(name)
}

// Formats the allowed range of a numeric parameter, e.g. "Range 0.00 … 5000.00 A"
function rangeString(conf, name) {
    if (conf.getParamEditAsPercentage(name)) {
        return ""
    }

    if (conf.isParamDouble(name)) {
        var scale = conf.getParamEditorScale(name)
        var dec = conf.getParamDecimalsDouble(name)
        return qsTr("Range %1 … %2%3").arg((conf.getParamMinDouble(name) * scale).toFixed(dec))
        .arg((conf.getParamMaxDouble(name) * scale).toFixed(dec)).arg(conf.getParamSuffix(name))
    }

    if (conf.isParamInt(name)) {
        return qsTr("Range %1 … %2%3").arg(conf.getParamMinInt(name))
        .arg(conf.getParamMaxInt(name)).arg(conf.getParamSuffix(name))
    }

    return ""
}
