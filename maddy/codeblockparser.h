/*
 * This project is licensed under the MIT license. For more information see the
 * LICENSE file.
 */
#pragma once

// -----------------------------------------------------------------------------

#include <functional>
#include <string>
#include <regex>

#include <LispHighlighter>
#include <QSyntaxStyle>

#include "maddy/blockparser.h"
#include "utility.h"

// -----------------------------------------------------------------------------

namespace maddy {

// -----------------------------------------------------------------------------

/**
 * CodeBlockParser
 *
 * From Markdown: 3 times surrounded code (without space in the beginning)
 *
 * ```
 *  ```
 * some code
 *  ```
 * ```
 *
 * To HTML:
 *
 * ```
 * <pre><code>
 * some code
 * </code></pre>
 * ```
 *
 * @class
 */
class CodeBlockParser : public BlockParser
{
public:
    /**
   * ctor
   *
   * @method
   * @param {std::function<void(std::string&)>} parseLineCallback
   * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} getBlockParserForLineCallback
   */
    CodeBlockParser(
            std::function<void(std::string&)> parseLineCallback,
            std::function<std::shared_ptr<BlockParser>(const std::string& line)> getBlockParserForLineCallback
            )
        : BlockParser(parseLineCallback, getBlockParserForLineCallback)
        , isStarted(false)
        , isFinished(false)
    {}

    /**
   * IsStartingLine
   *
   * If the line starts with three code signs, then it is a code block.
   *
   * ```
   *  ```
   * ```
   *
   * @method
   * @param {const std::string&} line
   * @return {bool}
   */
    static bool
    IsStartingLine(const std::string& line)
    {
        static std::regex re("^(?:`){3}(.*)$");
        return std::regex_match(line, re);
    }

    /**
   * IsFinished
   *
   * @method
   * @return {bool}
   */
    bool
    IsFinished() const override
    {
        return this->isFinished;
    }

protected:
    bool
    isInlineBlockAllowed() const override
    {
        return false;
    }

    bool
    isLineParserAllowed() const override
    {
        return false;
    }

    void
    parseBlock(std::string& line) override
    {
        if (!this->isStarted) {
            this->language = QString::fromStdString(line.substr(3)).trimmed();
            this->isStarted = true;
            line = "";
            return;
        }

        if (line != "```") {
            this->code.append(QString::fromStdString(line));
            line = "";
            return;
        }

        QString style = QString("\"background: %1; margin-left:12px; color:%2;\"").
                arg(Utility::getAppHexColor("lightBackground"), Utility::getAppHexColor("lightText"));
        QString cls = this->language.isEmpty() ? "" : " class=\"" + this->language.toHtmlEscaped() + "\"";

        line = "<pre style=" + style.toStdString() + cls.toStdString() +
                "><code><span style=\"font-family:'DejaVu Sans Mono';\">\n" +
                codeToHtml().toStdString() + "</span></code></pre>";
        this->isFinished = true;
    }

private:
    bool isStarted;
    bool isFinished;
    QString language;
    QStringList code;

    // Escapes the code and, for LispBM blocks, colors it like the Lisp editor
    QString
    codeToHtml() const
    {
        bool isLisp = this->language == "lisp" || this->language == "clj";
        QSyntaxStyle *syntaxStyle = Utility::isDarkMode() ?
                    QSyntaxStyle::darkStyle() : QSyntaxStyle::defaultStyle();
        QString html;

        for (const QString &text: this->code) {
            QVector<QString> formats(text.size());

            if (isLisp) {
                static const LispHighlighter highlighter;
                highlighter.forEachMatch(text, [&formats](int start, int length, const QString &format) {
                    std::fill(formats.begin() + start, formats.begin() + start + length, format);
                });
            }

            for (int start = 0, end = 0; start < text.size(); start = end) {
                while (end < text.size() && formats.at(end) == formats.at(start)) {
                    end++;
                }

                QString part = text.mid(start, end - start).toHtmlEscaped();
                QBrush color = syntaxStyle->getFormat(formats.at(start)).foreground();
                if (color.style() != Qt::NoBrush) {
                    part = QString("<span style=\"color:%1;\">%2</span>").arg(color.color().name(), part);
                }
                html += part;
            }

            html += "\n";
        }

        return html;
    }
}; // class CodeBlockParser

// -----------------------------------------------------------------------------

} // namespace maddy
