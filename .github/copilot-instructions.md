# Copilot instructions for cjong4

## Summary
- cjong4 is a C11 library implementing pure-function-oriented core and manager logic for 4-player Japanese mahjong (riichi mahjong).
- v3 uses `cj4_location locations[136]` as the canonical tile layout and reconstructs walls, hands, discards, melds, and dora indicators from it.

## Build, test, and lint commands
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON`
- Build: `cmake --build build --config Release --parallel`
- Test: `ctest --test-dir build -C Release --output-on-failure`
- Format check: `find include src tests -type f \( -name '*.c' -o -name '*.h' \) -print0 | xargs -0 clang-format --dry-run --Werror`
- Install: `cmake --install build --config Release --prefix /path/to/prefix`
- The tests use the repository's assertion-based `cj4_tests` executable and do not require an external test framework.

## High-level architecture (what Copilot should know)
- Position-based model: every physical tile has a fixed ID, and `locations[136]` is the canonical layout. Reverse lookups scan this array rather than using persistent duplicate arrays or caches.
- Pure functions: core logic is implemented as pure functions (no side effects, no hidden state). New functions should avoid global or static mutable state and be deterministic.
- State handling: functions should accept and return full state values (no in-place mutation).
- Tile identity: tiles are treated as unique entities (not only by type).
- 4-player specialization: player count is fixed at 4; turn order and hand sizes are fixed. Implementations should assume 4 players throughout.
- Language and targets: ISO C11 is the target standard. GCC, Clang, and MSVC are supported/expected.
- Public API prefixes: core symbols use `cj4_`; manager symbols use `cj4m_`.

## Key conventions and patterns
- No global or static mutable state: design functions to accept and return full state values rather than mutate shared state.
- Structural equality: equality should be structural/deterministic given the position-based model.
- Directory layout: public headers are under `include/cjong4/`, implementation files under `src/`, and tests under `tests/`.
- Naming: public APIs use `cj4_` prefix. Internal or private symbols may use a different convention but avoid exposing non-prefixed public names.
- Fixed sizes: hand sizes and player counts are fixed (13/14 tiles, 4 players). Data structures should be designed assuming these invariants (no need for dynamic sizing).

## Repository-specific notes
- CMake builds the static `cj4` library and optionally the `cj4_tests` executable with `BUILD_TESTS=ON`.
- Installed consumers use `find_package(cjong4 CONFIG REQUIRED)` and link `cjong4::cj4`.
- GitHub Actions validates GCC, Clang with ASan/UBSan, and MSVC.
- v3 is intentionally source- and ABI-incompatible with earlier releases; do not add compatibility aliases unless explicitly requested.

## How Copilot should assist
- Prefer solutions that maintain pure-function interfaces and avoid introducing global or static mutable state.
- When suggesting new files, follow the current layout and use the documented public symbol prefixes.
- Keep build and test instructions aligned with the top-level CMake configuration and README.

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
