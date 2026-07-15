# Contributing to Vexta Core

Thank you for your interest in contributing to Vexta Core.

Vexta Core is a Bitcoin Core–based blockchain focused on long-term stability, maintainability, and compatibility. Contributions that improve code quality, reliability, documentation, testing, or performance are welcome.

---

## Development Philosophy

The project follows a simple set of principles:

- Keep consensus changes to an absolute minimum.
- Stay as close to upstream Bitcoin Core as practical.
- Prefer simple, readable, and maintainable code.
- Remove legacy code instead of extending it.
- Preserve compatibility with standard Bitcoin ecosystem software whenever possible.

---

## Contributor Workflow

1. Fork the repository.
2. Create a feature branch.
3. Make focused, self-contained changes.
4. Ensure the project builds successfully.
5. Run the available tests.
6. Submit a pull request with a clear description.

Small pull requests are strongly preferred over large ones.

---

## Coding Guidelines

Please follow the existing Bitcoin Core coding style.

When making changes:

- avoid unnecessary complexity
- keep commits focused on a single task
- update documentation when appropriate
- remove obsolete code instead of leaving dead paths
- avoid introducing project-specific workarounds unless required

---

## Consensus Changes

Consensus changes require extra care.

Before proposing a consensus modification:

- document the motivation
- consider compatibility implications
- verify the impact on wallets, pools, explorers, and nodes
- include appropriate tests whenever practical

Consensus changes should remain rare.

---

## Pull Requests

Good pull requests generally:

- solve one problem
- build successfully
- include clear commit messages
- avoid unrelated formatting changes
- keep the diff as small as practical

---

## Code Review

Every change should be reviewed for:

- correctness
- readability
- maintainability
- performance
- security
- compatibility with upstream Bitcoin Core

---

## Release Process

Stable releases should only be created after:

- successful builds on supported platforms
- passing test suites
- review of consensus-related changes
- verification of release binaries

---

## License

By contributing to Vexta Core, you agree that your contributions will be distributed under the MIT License.
