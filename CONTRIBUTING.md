<!--------------------------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
----------------------------------------------------------------------------------------------------->

# Contributing

Thank you for your interest in contributing to the MQSS Quantum Compilation Suite! This document
outlines how to get set up and how to contribute.

We use GitHub to host code, track issues and feature requests, and accept pull requests. See
<https://docs.github.com/en/get-started/quickstart> for a general introduction to contributing on
GitHub.

## Ways to Contribute

- **Report bugs**: Open an issue describing steps to reproduce, expected vs. actual behavior, and
  your environment (OS/architecture, whether you're using the devcontainer).
- **Fix bugs / implement passes**: Browse open issues, especially anything labeled "bug" or "good
  first issue". For non-trivial changes, open a draft PR early to get feedback before investing a
  lot of time.
- **Propose features**: New passes, dialect support, or pipeline changes should ideally be discussed
  in an issue first, since they often interact with both the `cudaq-quake` and `catalyst-quantum`
  dialects.
- **Improve documentation**: Fixes to `docs/`, the `README.md`, or inline comments are always
  welcome, including small documentation-only PRs.
- **Add tests**: The dialect-level (`tests/dialects`) and end-to-end (`tests/code`) test suites are
  the main way we verify pass correctness — additional lit/FileCheck coverage is valuable on its
  own.

## Guidelines

- Focus each PR on a single pass, bug, or feature; split unrelated changes into separate PRs.
- Write meaningful commit messages.
- Add or update tests (`tests/dialects` for dialect-level checks, `tests/code` for end-to-end
  `mqss-cc` checks) for any new feature or bug fix.
- Keep your branch clean: remove debug statements, leftover comments, and unrelated changes before
  requesting review.
- Run the pre-commit hooks (see [Code Style](#code-style-and-linting)) before pushing.
- Be responsive to review feedback and open to changes.

### AI-assisted contributions

AI coding agents (e.g., Claude Code, GitHub Copilot) can be used to help prepare contributions, but
**you are responsible for every line of code you submit** and a human must review and understand the
change before it is opened for review. Please disclose AI assistance in the PR description when it
was used for non-trivial parts of the change.

## Commit Messages and Versioning

Formatted commit messages and consistent versioning are used to make the project history readable
and suitable for automated processing.

### Commit Messages

Commits follow a format inspired by [Conventional Commits](https://www.conventionalcommits.org),
using square brackets to indicate scope or type.

```
<emoji> [<scope|type>] <Short description>

[Optional: Long description]
```

#### Short Description

- Use square brackets to categorize:
  - `[scope]` for individual commits (e.g., `[ui]`, `[core]`, `[backend]`)
  - `[type]` for PRs/merges (e.g., `[feat]`, `[fix]`, `[refactor]`)
- Use imperative mood, starting with a capital letter (e.g., "Add feature" instead of "Added
  feature")
- Keep it concise (72 characters or fewer recommended)
- Emoji is optional

#### Long Description (optional)

- Add a blank line after the header
- Provide context, especially for non-trivial changes
- Use plain text or bullet points for multiple details
- Keep line length reasonable (100 characters or fewer recommended)

### Versioning

Releases follow [Semantic Versioning (SemVer)](https://semver.org) and are tracked using Git tags in
the format `vMAJOR.MINOR.PATCH`:

- **MAJOR** - incompatible API changes
- **MINOR** - backward-compatible feature additions
- **PATCH** - backward-compatible bug fixes

### Improvements and Open Points

The conventions described above may change over time.

Open topics may include:

- Use of `:` as a separator in the header, e.g., `[scope]: description`
- Refinement of scopes/types, e.g, should the set of allowed `[scope]` and `[type]` values be
  restricted and documented?
- Tooling and automation, e.g., should commit message formatting be enforced via tooling (e.g.,
  commit linting)?
- Include both type and scope:
  - `[type][scope]:` - consistent with current bracket-based style
  - `type(scope):` - closer to Conventional Commits and existing tooling

## Development Environment

This project is developed inside the provided Docker devcontainer (Docker + VS Code + the Dev
Containers extension). See [docs/user_guide/build.md](docs/user_guide/build.md) for the full
build/test workflow; the short version:

```bash
make build                       # configure (downloads LLVM/MLIR, fetches CUDA-Q/Catalyst)
make target                      # build mqss-opt with Ninja
eval "$(make set-target-paths)"  # put mqss-opt on PATH
make test-dialects                # run the dialect-level lit/FileCheck tests
```

For end-to-end tests that also exercise `cudaq-quake`/`mqss-cc`, run `make frontend`,
`eval "$(make front-end-paths)"`, then `make test-all`. See
[docs/develop-guide/develop-guide.md](docs/develop-guide/develop-guide.md) for how the dialect-level
and end-to-end lit tests are structured, and how to write new ones.

## Code Style and Linting

C++ code follows the [LLVM Coding Standard](https://llvm.org/docs/CodingStandards.html), enforced
via:

- [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) (config in `.clang-format`)
- [`clang-tidy`](https://clang.llvm.org/extra/clang-tidy/) (config in `.clang-tidy`) — CI
  (`clang-tidy.yml`) runs it against the lines changed in your PR and comments with any findings
- `cmake-format` for `CMakeLists.txt`/`*.cmake` files

Install the [pre-commit](https://pre-commit.com/) hooks once, and they'll run automatically on every
commit:

```bash
pip install pre-commit
pre-commit install
```

Or run them manually against the whole tree:

```bash
pre-commit run --all-files
```

The same checks run in CI via `pre-commit.yml` and must pass before merging.

## Documentation

Docs live in `docs/` and are built with Sphinx (MyST Markdown). Build them locally with:

```bash
pip install sphinx myst-parser furo
sphinx-build docs _build/html
```

Then open `_build/html/index.html` in a browser. On merge to `develop`, `docs.yml` runs the same
command and deploys the result to GitHub Pages automatically.

## Pull Request Workflow

- Target the `develop` branch.
- Draft PRs are welcome for work in progress.
- CI runs three checks on every PR: `ci.yml` (build `mqss-opt` and run `make test-dialects`),
  `clang-tidy.yml` (lint changed C++ lines), and `pre-commit.yml` (formatting/lint hooks). All must
  pass before merging.
- Address review feedback by pushing new commits to the same branch rather than opening a new PR.
- Avoid force-pushing/rebasing before review is complete; rebasing afterward to clean up history is
  fine.

## Contact

Development is led by the QCT department at LRZ and the QSI department at MQV gGmbH. You can also
reach us at <mqss@munich-quantum-valley.de>. Please prefer public GitHub channels (issues,
discussions, PRs) over private contact so discussion stays transparent.
