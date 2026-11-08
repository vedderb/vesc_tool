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

#include "mcpparamvalidator.h"

#include <cmath>
#include <limits>

bool McpParamValidator::validate(const ConfigParam &param, const QJsonValue &value, QString *error)
{
    bool valid = false;
    switch (param.type) {
    case CFG_T_DOUBLE:
        valid = value.isDouble() && std::isfinite(value.toDouble()) &&
                value.toDouble() >= param.minDouble && value.toDouble() <= param.maxDouble;
        break;
    case CFG_T_INT:
    case CFG_T_ENUM: {
        const double number = value.toDouble(std::numeric_limits<double>::quiet_NaN());
        valid = value.isDouble() && std::isfinite(number) && std::floor(number) == number &&
                number >= std::numeric_limits<int>::min() &&
                number <= std::numeric_limits<int>::max();
        if (valid) {
            valid = param.type == CFG_T_ENUM ? number >= 0 && number < param.enumNames.size()
                                             : number >= param.minInt && number <= param.maxInt;
        }
    } break;
    case CFG_T_BOOL:
        valid = value.isBool();
        break;
    case CFG_T_QSTRING:
        valid = value.isString() && value.toString().size() <= param.maxLen &&
                !value.toString().contains(QChar(0));
        break;
    default:
        *error = "This parameter type is not supported for MCP edits.";
        return false;
    }
    if (!valid) {
        *error = "Value has the wrong type or is outside the parameter limits.";
    }
    return valid;
}
