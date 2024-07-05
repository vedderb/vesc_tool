/*
 * This project is licensed under the MIT license. For more information see the
 * LICENSE file.
 */
#pragma once

// -----------------------------------------------------------------------------

#include <functional>
#include <string>
#include <QDebug>

#include "maddy/blockparser.h"
#include "maddy/codeblockparser.h"
#include "maddy/headlineparser.h"
#include "maddy/horizontallineparser.h"
#include "maddy/quoteparser.h"
#include "maddy/tableparser.h"
#include "maddy/checklistparser.h"
#include "maddy/orderedlistparser.h"
#include "maddy/unorderedlistparser.h"
#include "maddy/htmlparser.h"

// -----------------------------------------------------------------------------

namespace maddy {

// -----------------------------------------------------------------------------

/**
 * ParagraphParser
 *
 * @class
 */
class ParagraphParser : public BlockParser
{
public:
  /**
   * ctor
   *
   * @method
   * @param {std::function<void(std::string&)>} parseLineCallback
   * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} getBlockParserForLineCallback
   */
   ParagraphParser(
    std::function<void(std::string&)> parseLineCallback,
    std::function<std::shared_ptr<BlockParser>(const std::string& line)> getBlockParserForLineCallback,
    bool isEnabled
  )
    : BlockParser(parseLineCallback, getBlockParserForLineCallback)
    , isStarted(false)
    , isFinished(false)
    , isEnabled(isEnabled)
  {}

  /**
   * IsStartingLine
   *
   * If the line is not empty, it will be a paragraph.
   *
   * This block parser has to always run as the last one!
   *
   * @method
   * @param {const std::string&} line
   * @return {bool}
   */
  static bool
  IsStartingLine(const std::string& line)
  {
    return !line.empty();
  }

  /**
   * IsFinished
   *
   * An empty line will end the paragraph.
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
    return true;
  }

  void parseBlock(std::string& line) override {
      // Check if another parser should take over
      if (this->isStarted && (
                  maddy::CodeBlockParser::IsStartingLine(line) ||
                  maddy::HeadlineParser::IsStartingLine(line) ||
                  maddy::HorizontalLineParser::IsStartingLine(line) ||
                  maddy::QuoteParser::IsStartingLine(line) ||
                  maddy::TableParser::IsStartingLine(line) ||
                  maddy::ChecklistParser::IsStartingLine(line) ||
                  maddy::OrderedListParser::IsStartingLine(line) ||
                  maddy::UnorderedListParser::IsStartingLine(line) ||
                  maddy::HtmlParser::IsStartingLine(line)
                  )) {
          this->reparseLine = line;
          line = this->isEnabled ? "</p>" : "<br/>";
          this->isFinished = true;
          return;
      }

      if (!this->isStarted) {
          if (this->isEnabled) {
              line = "<p>" + line + " ";
          } else {
              line += " ";
          }

          this->isStarted = true;
      } else {
          if (line.empty()) {
              line += this->isEnabled ? "</p>" : "<br/>";
              this->isFinished = true;
          } else {
              line += " ";
          }
      }
  }

private:
  bool isStarted;
  bool isFinished;
  bool isEnabled;

}; // class ParagraphParser

// -----------------------------------------------------------------------------

} // namespace maddy
