# Changelog

This file tries to follow roughly [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
maddy uses [semver versioning](https://semver.org/).

## Badges

* ![**FIXED**](https://img.shields.io/badge/-FIXED-%23090) for any bug fixes.
* ![**SECURITY**](https://img.shields.io/badge/-SECURITY-%23c00) in case of vulnerabilities.
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) for new features.
* ![**CHANGED**](https://img.shields.io/badge/-CHANGED-%23e90) for changes in existing functionality.
* ![**DEPRECATED**](https://img.shields.io/badge/-DEPRECATED-%23666) for soon-to-be removed features.
* ![**REMOVED**](https://img.shields.io/badge/-REMOVED-%23900) for now removed features.

## Upcoming

* ?

## version 1.3.0 2023-08-26

* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Headlines can have inline parsing now. It is on by default, but can be disabled by config.

## version 1.2.1 2023-08-06

* ![**FIXED**](https://img.shields.io/badge/-FIXED-%23090) Parser.h version() method clashing with VERSION defines at global scope

## version 1.2.0 2023-07-27

* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Added Changelog
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Added contribution guideline
* ![**CHANGED**](https://img.shields.io/badge/-CHANGED-%23e90) updated cmake minimum required version to 3.25
* ![**CHANGED**](https://img.shields.io/badge/-CHANGED-%23e90) gtest is now loaded via cmake and not a git submodule any longer - updated gtest version to 1.13.0
* ![**CHANGED**](https://img.shields.io/badge/-CHANGED-%23e90) tests are only run if the cmake option `MADDY_BUILD_WITH_TESTS` is on, moved test cmake code to the `tests` subfolder
* ![**REMOVED**](https://img.shields.io/badge/-REMOVED-%23900) travis CI and appveyor
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) GitHub workflow for tests
* ![**DEPRECATED**](https://img.shields.io/badge/-DEPRECATED-%23666) config flags `isEmphasizedParserEnabled` and `isHTMLWrappedInParagraph`
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) config flag `enabledParsers` to en-/disable each parser separately
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) class attribute to code blocks if there is text after the three backticks like ` ```cpp`
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) optional support for latex blocks - it's off by default
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) version info to the parser class
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) GitHub workflow for release, so that one can include maddy easier via cmake's `FetchContent`

## version 1.1.2 2020-10-04

* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) `*`, `+` and `-` are equivalent for making unordered bullet list
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Parsing support for fully numeric ordered lists
* ![**CHANGED**](https://img.shields.io/badge/-CHANGED-%23e90) make `Parser::Parse` accept istreams instead of stringstream
* ![**CHANGED**](https://img.shields.io/badge/-CHANGED-%23e90) CMake is creating an interface library which you can include in your own `target_link_libraries` and the global include path is untouched from maddy.

## version 1.1.1 2019-12-27

* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) BreakLineParser
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) HTMLParser
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Added optional config with the following options:
    * en-/disable the emphasized parser
    * wrap/not wrap HTML in markdown within a paragraph in output
* ![**CHANGED**](https://img.shields.io/badge/-CHANGED-%23e90) Updated gtest to release-1.10.0 to fix build issues


## version 1.1.0 2019-02-19

* ![**FIXED**](https://img.shields.io/badge/-FIXED-%23090) Added missing includes to BlockParser
* ![**FIXED**](https://img.shields.io/badge/-FIXED-%23090) Added missing dtor to BlockParser and LineParser
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) `__test__` can also be used to get `<strong>text</strong>`
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Added AppVeyor CI
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Added clang for CI
* ![**CHANGED**](https://img.shields.io/badge/-CHANGED-%23e90) Single underscore `_` results in emphasized tag `<em>`, single `*` in italic tag `<i>`

## version 1.0.3 2018-01-18

* ![**FIXED**](https://img.shields.io/badge/-FIXED-%23090) Make sure that all parsers are finished
* ![**FIXED**](https://img.shields.io/badge/-FIXED-%23090) ol documentation
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Added Travic-CI with gcc
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Added Howto for running the tests on the README

## version 1.0.2 2017-12-26

* ![**FIXED**](https://img.shields.io/badge/-FIXED-%23090) Fixed inline code for directly following letters (bold, emphasized and strikethrough)

## version 1.0.1 2017-12-25

* ![**FIXED**](https://img.shields.io/badge/-FIXED-%23090) Fixed inline code for bold, emphasized and strikethrough
* ![**FIXED**](https://img.shields.io/badge/-FIXED-%23090) Fixed spelling in README
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Use Gold Linker on Unix if available for faster compile time
* ![**ADDED**](https://img.shields.io/badge/-ADDED-%23099) Added Github ISSUE_TEMPLATE

## version 1.0.0 2017-12-25

initial release
