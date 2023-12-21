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
 * StrongParser
 *
 * Has to be used before the `EmphasizedParser`.
 *
 * @class
 */
class StrongParser : public LineParser
{
public:
  /**
   * Parse
   *
   * From Markdown: `text **text** __text__`
   *
   * To HTML: `text <strong>text</strong> <strong>text</strong>`
   *
   * @method
   * @param {std::string&} line The line to interpret
   * @return {void}
   */
  void
  Parse(std::string& line) override
  {
      std::string pattern = "**";
      std::string newPattern = "strong";

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

      pattern = "__";

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
}; // class StrongParser

// -----------------------------------------------------------------------------

} // namespace maddy
