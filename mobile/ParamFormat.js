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
