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
 * EmphasizedParser
 *
 * Has to be used after the `StrongParser`.
 *
 * @class
 */
class EmphasizedParser : public LineParser
{
public:
  /**
   * Parse
   *
   * From Markdown: `text _text_`
   *
   * To HTML: `text <em>text</em>`
   *
   * @method
   * @param {std::string&} line The line to interpret
   * @return {void}
   */
  void
  Parse(std::string& line) override
  {
      std::string pattern = "_";
      std::string newPattern = "em";

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
}; // class EmphasizedParser

// -----------------------------------------------------------------------------

} // namespace maddy
