# Copilot instructions for cjong4

## Summary
- cjong4 is a pure-functional C library implementing 4-player (riichi) mahjong core logic. Current repository only contains README and LICENSE; project is WIP (no build/test scripts detected).

## Build, test, and lint commands
- No build, test, or lint scripts detected in the repository root.
- If adding a minimal local build, prefer a small Makefile or CMake targeting C11. Example manual commands (illustrative only, not present in repo):
  - Build: gcc -std=c11 -Iinclude -c src/*.c && ar rcs libcj4.a *.o
  - Single-file build: gcc -std=c11 -Iinclude -o example_bin examples/example.c src/*.c
  - Lint (if added): run clang-tidy / cppcheck configured for C11
  - Tests: no test harness found. Recommend adding a C test framework (e.g., Check, Unity) and expose how to run a single test via the test runner (e.g., ./tests/test_runner --filter test_name).

## High-level architecture (what Copilot should know)
- Position-based model: every tile has a fixed position; state is represented as a layout. Expect fixed-size arrays/indexed layouts where positions are part of the state model, rather than multiset counters.
- Pure functions: core logic is implemented as pure functions (no side effects, no hidden state). New functions should avoid global or static mutable state and be deterministic.
- State handling: functions should accept and return full state values (no in-place mutation).
- Tile identity: tiles are treated as unique entities (not only by type).
- 4-player specialization: player count is fixed at 4; turn order and hand sizes are fixed. Implementations should assume 4 players throughout.
- Language and targets: ISO C11 is the target standard. GCC, Clang, and MSVC are supported/expected.
- Public API prefix: public symbols use `cj4_` prefix (e.g., `cj4_state`, `cj4_tile`, `cj4_apply_action`). Keep public API names consistent.

## Key conventions and patterns
- No global or static mutable state: design functions to accept and return full state values rather than mutate shared state.
- Structural equality: equality should be structural/deterministic given the position-based model.
- Directory layout (planned): include/, cjong4/, src/ — follow this convention when adding headers and sources.
- Naming: public APIs use `cj4_` prefix. Internal or private symbols may use a different convention but avoid exposing non-prefixed public names.
- Fixed sizes: hand sizes and player counts are fixed (13/14 tiles, 4 players). Data structures should be designed assuming these invariants (no need for dynamic sizing).

## Repository-specific notes
- Current repository is in early WIP state; README documents design principles and planned features. When adding tooling, include top-level Makefile or CMakeLists.txt and a test runner so Copilot can recommend exact commands.
- No AI assistant-specific config files (CLAUDE.md, AGENTS.md, .cursorrules, .windsurfrules, CONVENTIONS.md, etc.) were found. If added, merge important guidance into this file.

## How Copilot should assist
- Prefer solutions that maintain pure-function interfaces and avoid introducing global or static mutable state.
- When suggesting new files, follow the planned layout and prefix public symbols with `cj4_`.
- For build/test suggestions, prefer adding explicit scripts (Makefile/CMake/test runner) and document single-test invocation.

## Notes
- Keep implementations simple and deterministic rather than abstract or generic.
- Do not introduce unnecessary dynamic allocation if fixed-size structures are sufficient.

## Safety / Constraints for Copilot

- Do NOT create commits automatically.
- Do NOT run git commands (commit, push, rebase, etc.).
- Do NOT download or install any external tools, packages, or dependencies.
- Do NOT execute shell commands that modify the environment.

## Code generation constraints

- Only modify files when explicitly instructed by the user.
- Prefer showing diffs or full file content instead of applying changes automatically.
- Do not introduce new build systems or dependencies unless explicitly requested.
- Keep the project self-contained (no external libraries unless specified).

## Environment constraints

- Assume a local development environment without internet access.
- Do not attempt to fetch remote resources or install anything.

## Language

- All responses must be written in Japanese.
- Use clear and concise technical Japanese.
- Keep code comments in English unless otherwise specified.