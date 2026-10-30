# ZetaX Agent Guidance

## Repository Rules

- Before modifying code, briefly explain the planned change.
- Do not run dangerous commands automatically.
- Do not modify `3rd/`, `third_party/`, `external/`, `vendor/`, or generated
  files unless the user explicitly asks.
- Prefer small, focused changes.
- Before committing, inspect `git status` and `git diff`.
- Do not run full builds unless the user explicitly requests one.
- Prefer lightweight verification first and report what was and was not run.
- Update relevant documentation with changes to behavior, APIs, configuration,
  operations, or user-visible output. Otherwise, state why no update was needed.

## Skill Routing

- ZetaX programming, design, review, dependencies, source investigation, and
  documentation organization: `$zetax-project`.
- All code work: `$zetax-code-style` plus matching language skills:
  `$zetax-cpp-style` for C/C++, CMake and Conan; `$zetax-python-style` for
  Python, packaging and automation. Apply all languages in mixed work.
- New subsystems, large features, module/dependency/lifecycle boundaries,
  extensibility, source concentration or cross-module refactoring:
  `$zetax-architecture`.
- Roadmaps, Phase/Stage decomposition, acceptance and delivery mapping:
  `$zetax-planning`.
- Commits, amends, staged reviews and messages: `$zetax-commit`. Use one bounded
  workflow with version updates by default. Amend only the current branch's
  unpushed `HEAD` when explicitly requested. Commit-only work does not activate
  implementation/review workflows.

Read only references relevant to the task. Reuse unchanged guidance already
available in the current context; reread after changes or context loss.

## Precedence

1. Existing project configuration and executable tooling
2. Language-specific ZetaX standard
3. ZetaX general programming standard
4. Common ecosystem convention

## zgent Collaboration

- zgent follows all ZetaX project conventions and constraints. Its distinguishing
  choice is user-led development, not an exemption from shared standards.
- The user leads the core framework, key logic, design direction, and idea
  validation, maintaining a complete understanding of the code framework.
- Agents normally assist with auxiliary code, documentation, build integration,
  and focused verification around the user's code and stated direction.
- For changes affecting the main framework or key design, explain the basis
  and impact and follow the user's explicit direction. Do not independently
  expand architecture or adopt zeta_agent features or milestones as requirements.
- Carry out explicitly delegated work within its authorized scope. This division
  of responsibility does not introduce approval for every routine step.
- Preserve small, understandable changes and clearly distinguish implemented
  behavior, design proposals, source observations, and runtime verification.
- Use Org for project documentation, English as primary, and synchronized
  Simplified Chinese pages. Keep build/runtime command details in `BUILD.org`.

See [English development guide](doc/en/development.org) and
[中文开发指南](doc/zh-CN/development.org) for the source reading path,
repository ownership, and verification rules.
