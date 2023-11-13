/*
 * This project is licensed under the MIT license. For more information see the
 * LICENSE file.
 */
#pragma once

// -----------------------------------------------------------------------------

#include <string>
#include <regex>

#include "maddy/lineparser.h"

// -----------------------------------------------------------------------------

namespace maddy {

// -----------------------------------------------------------------------------

/**
 * StrikeThroughParser
 *
 * @class
 */
class StrikeThroughParser : public LineParser
{
public:
  /**
   * Parse
   *
   * From Markdown: `text ~~text~~`
   *
   * To HTML: `text <s>text</s>`
   *
   * @method
   * @param {std::string&} line The line to interpret
   * @return {void}
   */
  void
  Parse(std::string& line) override
  {
      std::string pattern = "~~";
      std::string newPattern = "s";

      for (;;) {
          int patlen = pattern.size();

          auto pos1 = line.find(pattern);
          if (pos1 == std::string::npos) {
              break;
          }

          auto pos2 = line.find(pattern, pos1 + patlen);
          if (pos2 == std::string::npos) {
              break;
          }

          std::string word = line.substr(pos1 + patlen, pos2 - pos1 - patlen);
          line = line.replace(pos1, (patlen + pos2) - pos1, "<" + newPattern + ">" + word + "</" + newPattern + ">");
      }
  }
}; // class StrikeThroughParser

// -----------------------------------------------------------------------------

} // namespace maddy
