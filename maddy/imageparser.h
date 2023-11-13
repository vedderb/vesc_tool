/*
 * This project is licensed under the MIT license. For more information see the
 * LICENSE file.
 */
#pragma once

// -----------------------------------------------------------------------------

#include <string>

#include "maddy/lineparser.h"

#include <QStandardPaths>
#include <QRegularExpression>
#include <QFile>
#include <QFileInfo>
#include "utility.h"

// -----------------------------------------------------------------------------

namespace maddy {

// -----------------------------------------------------------------------------

/**
 * ImageParser
 *
 * Has to be used before the `LinkParser`.
 *
 * @class
 */
class ImageParser : public LineParser
{
public:
    /**
   * Parse
   *
   * From Markdown: `![text](http://example.com/a.png)`
   *
   * To HTML: `<img src="http://example.com/a.png" alt="text"/>`
   *
   * @method
   * @param {std::string&} line The line to interpret
   * @return {void}
   */
    void
    Parse(std::string& line) override
    {
        static QRegularExpression req(R"(\!\[([^\]]*)\]\(([^\]]*)\))");
        QRegularExpressionMatch match = req.match(QString::fromStdString(line));

        if (match.hasMatch()) {
            QString txt = match.captured(1);
            QString imgPath = match.captured(2);

            if (!QFileInfo::exists(imgPath)) {
                if (QUrl(imgPath).isValid()) {
                    QString cacheLoc = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
                    QString cachePath = imgPath;

                    int indLastDot = cachePath.lastIndexOf(".");
                    if (indLastDot > 0) {
                        QString beginning = cachePath.mid(0, indLastDot);
                        QString ending = cachePath.mid(indLastDot);
                        QByteArray bytes = beginning.toUtf8();
                        beginning = QString::number(Utility::crc32c((uint8_t*)bytes.data(), bytes.size()));
                        cachePath = beginning + ending;
                    }

                    cachePath = cacheLoc + "/img/" + cachePath;

                    if (!QFileInfo::exists(cachePath)) {
                        QDir().mkpath(cacheLoc + "/img/");
                        Utility::downloadUrlEventloop(imgPath, cachePath);
                    }

                    if (QFileInfo::exists(cachePath)) {
                        imgPath = cachePath;
                    }
                }
            }

            QString update = QString::fromStdString(line).
                    replace(req, QString("<img src=\"%1\" alt=\"%2\"/>").arg(imgPath, txt));
            line = update.toStdString();
        }
    }
}; // class ImageParser

// -----------------------------------------------------------------------------

} // namespace maddy
