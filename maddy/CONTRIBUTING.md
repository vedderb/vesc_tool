# Contribution Guideline

First of all: I am thankful for any contribution this project gets.

## Creating Issues

You found a bug, you miss some feature in the project or have an idea how to
improve the code? Then [create a GitHub issue](https://github.com/progsource/maddy/issues/new).

## Creating Pull-Requests

* Use a branch other than master.
* Add yourself to the `AUTHORS` file.
* Try to stick with the code style the files are having right now.
* Write in your commit messages what/why you did something. Often times a one-liner might be enough, but if you want to write more, make an empty line in between like:
  ```
  Short description

  More and longer text for the commit message with some more information.
  That can go over multiple lines.
  ```
  Do not include Github issue ticket numbers inside commit messages.
* Explain for what your PR is for - like providing a use-case or something similar.
* Update documentation of the Markdown syntax if anything changed there. (`docs/definitions.md`)
* Add a changelog entry at "Upcoming" inside of `CHANGELOG.md`
* Make sure, that the tests are successful.
