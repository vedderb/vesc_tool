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


#include "datatypes.h"
#include "utility.h"

#include <QByteArray>
#include <QString>

QString VescPackage::iconSrc()
{
    const QByteArray &rawBytes = Utility::isDarkMode() ? icon : iconLight;

    if (!rawBytes.isEmpty()) {
        QByteArray mime = "image/*"; // Wildcard fallback

        if (rawBytes.startsWith("\xFF\xD8\xFF")) {
            mime = "image/jpeg";
        } else if (rawBytes.startsWith("\x89PNG\r\n\x1A\n")) {
            mime = "image/png";
        } else if (rawBytes.startsWith("GIF87a") || rawBytes.startsWith("GIF89a")) {
            mime = "image/gif";
        } else if (rawBytes.startsWith("RIFF") && rawBytes.mid(8, 4) == "WEBP") {
            mime = "image/webp";
        } else if (rawBytes.startsWith("BM")) {
            mime = "image/bmp";
        } else if (rawBytes.trimmed().startsWith("<svg") || rawBytes.trimmed().startsWith("<?xml")) {
            mime = "image/svg+xml";
        }

        return QString::fromLatin1("data:" + mime + ";base64," + rawBytes.toBase64());
    }

    return "qrc" + Utility::getThemePath() + "icons/Package-96.png";
}
