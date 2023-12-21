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

// -----------------------------------------------------------------------------

namespace maddy {

// -----------------------------------------------------------------------------

/**
 * LatexBlockParser
 *
 * Support for https://www.mathjax.org/
 * Be aware, that if you want to make MathJax work, you need also their
 * JavaScript library added to your HTML code.
 * maddy does not itself add that code to be more flexible in how you write your
 * head and full body.
 *
 * From Markdown: `$$` surrounded text
 *
 * ```
 *  $$some formula
 *  $$
 * ```
 *
 * To HTML:
 *
 * ```
 * $$some formula
 * $$
 * ```
 *
 * @class
 */
class LatexBlockParser : public BlockParser
{
public:
  /**
   * ctor
   *
   * @method
   * @param {std::function<void(std::string&)>} parseLineCallback
   * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} getBlockParserForLineCallback
   */
   LatexBlockParser(
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
   * If the line starts with two dollars, then it is a latex block.
   *
   * ```
   *  $$
   * ```
   *
   * @method
   * @param {const std::string&} line
   * @return {bool}
   */
  static bool
  IsStartingLine(const std::string& line)
  {
    static std::regex re(R"(^(?:\$){2}(.*)$)");
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
    if (!this->isStarted && line.substr(0, 2) == "$$")
    {
      this->isStarted = true;
      this->isFinished = false;
    }

    if (this->isStarted && !this->isFinished && line.size() > 1 && line.substr(line.size() - 2, 2) == "$$")
    {
      this->isFinished = true;
      this->isStarted = false;
    }

    line += "\n";
  }

private:
  bool isStarted;
  bool isFinished;
}; // class LatexBlockParser

// -----------------------------------------------------------------------------

} // namespace maddy
