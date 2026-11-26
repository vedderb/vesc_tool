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
 * TableParser
 *
 * From Markdown (GitHub style, the second row sets the column alignment):
 *
 * ```
 * | Name | Value |
 * |:-----|------:|
 * | a    | 1     |
 * ```
 *
 * To HTML:
 *
 * ```
 * <table><thead><tr><th align="left">Name</th>...</tr></thead>
 * <tbody><tr><td align="left">a</td>...</tr></tbody></table>
 * ```
 *
 * Lines starting with `|` without that second row become a paragraph.
 *
 * @class
 */
class TableParser : public BlockParser
{
public:
  /**
   * ctor
   *
   * @method
   * @param {std::function<void(std::string&)>} parseLineCallback
   * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} getBlockParserForLineCallback
   */
   TableParser(
    std::function<void(std::string&)> parseLineCallback,
    std::function<std::shared_ptr<BlockParser>(const std::string& line)> getBlockParserForLineCallback
  )
    : BlockParser(parseLineCallback, getBlockParserForLineCallback)
    , isFinished(false)
  {}

  /**
   * IsStartingLine
   *
   * Every table row starts with `|`.
   *
   * @method
   * @param {const std::string&} line
   * @return {bool}
   */
  static bool
  IsStartingLine(const std::string& line)
  {
    return !line.empty() && line[0] == '|';
  }

  /**
   * AddLine
   *
   * Collects rows until a line that does not start with `|`, which is then
   * handed back to the parser.
   *
   * @method
   * @param {std::string&} line
   * @return {void}
   */
  void
  AddLine(std::string& line) override
  {
    if (IsStartingLine(line))
    {
      this->lines.push_back(line);
      return;
    }

    this->reparseLine = line;
    this->parseBlock(line);
    this->isFinished = true;
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
    return true;
  }

  void
  parseBlock(std::string&) override
  {
    std::vector<std::string> header = splitRow(this->lines[0]);
    std::vector<std::string> align;

    if (this->lines.size() > 1)
    {
      static std::regex delimiter("^:?-+:?$");

      for (const std::string& cell : splitRow(this->lines[1]))
      {
        if (!std::regex_match(cell, delimiter))
        {
          align.clear();
          break;
        }

        bool left = cell.front() == ':';
        bool right = cell.back() == ':';
        align.push_back(
          left && right ? " align=\"center\"" :
          right ? " align=\"right\"" :
          left ? " align=\"left\"" : ""
        );
      }
    }

    if (align.empty() || align.size() != header.size())
    {
      result << "<p>";
      for (std::string line : this->lines)
      {
        this->parseLine(line);
        result << line << " ";
      }
      result << "</p>";
      return;
    }

    result << "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\" style=\"border-collapse: collapse; border-style: solid; border-color: "
           << Utility::getAppHexColor("lightestBackground").toStdString() << ";\">";

    for (size_t row = 0; row < this->lines.size(); row++)
    {
      if (row == 1)
      {
        continue;
      }

      std::vector<std::string> cells = row == 0 ? header : splitRow(this->lines[row]);
      const char* tag = row == 0 ? "th" : "td";

      result << (row == 0 ? "<thead><tr>" : "<tr>");

      for (size_t col = 0; col < header.size(); col++)
      {
        std::string cell = col < cells.size() ? cells[col] : "";
        this->parseLine(cell);
        result << "<" << tag << align[col] << ">" << cell << "</" << tag << ">";
      }

      result << (row == 0 ? "</tr></thead><tbody>" : "</tr>");
    }

    result << "</tbody></table>";
  }

private:
  bool isFinished;
  std::vector<std::string> lines;

  // Splits "| a | b \| c |" into {"a", "b | c"}
  static std::vector<std::string>
  splitRow(const std::string& line)
  {
    std::vector<std::string> cells;
    std::string cell;

    for (size_t i = 1; i <= line.size(); i++)
    {
      if (i < line.size() && line[i] == '\\' && i + 1 < line.size() && line[i + 1] == '|')
      {
        cell += '|';
        i++;
      }
      else if (i == line.size() || line[i] == '|')
      {
        size_t first = cell.find_first_not_of(" \t");
        cell = first == std::string::npos ? "" :
          cell.substr(first, cell.find_last_not_of(" \t") - first + 1);

        if (i < line.size() || !cell.empty())
        {
          cells.push_back(cell);
        }

        cell.clear();
      }
      else
      {
        cell += line[i];
      }
    }

    return cells;
  }
}; // class TableParser

// -----------------------------------------------------------------------------

} // namespace maddy
