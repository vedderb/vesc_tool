/*
 * This project is licensed under the MIT license. For more information see the
 * LICENSE file.
 */
#pragma once

// -----------------------------------------------------------------------------

#include <functional>
#include <string>
#include <regex>

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
        QString style = QString("\"background: %1; margin-left:12px; color:%2;\"").
                arg(Utility::getAppHexColor("lightBackground"), Utility::getAppHexColor("lightText"));

        if (line == "```") {
            if (!this->isStarted) {
                line = "<pre style=" + style.toStdString() +
                        "><span style=\"font-family:'DejaVu Sans Mono';\"<code>\n";
                this->isStarted = true;
                this->isFinished = false;
                return;
            } else {
                line = "</span></code></pre>";
                this->isFinished = true;
                this->isStarted = false;
                return;
            }
        } else if (!this->isStarted && line.substr(0, 3) == "```") {
            line = "<pre style=" + style.toStdString() + " class=\"" +
                    line.substr(3) + "\"><span style=\"font-family:'DejaVu Sans Mono';\"<code>\n";
            this->isStarted = true;
            this->isFinished = false;
            return;
        }

        line += "\n";
    }

private:
    bool isStarted;
    bool isFinished;
}; // class CodeBlockParser

// -----------------------------------------------------------------------------

} // namespace maddy
