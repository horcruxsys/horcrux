# AGENTS.md — Horcrux

C++23 universal build system. CMake 3.25+ with Ninja.

## Quick start

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --output-on-failure -j$(nproc)
# If you see spurious failures, run sequentially:
ctest --output-on-failure -j1
```

## Key commands

| Action | Command |
|--------|---------|
| Configure | `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug` |
| Build | `cmake --build build -j$(nproc)` |
| Run all tests | `ctest --output-on-failure -j$(nproc)` (from `build/`) |
| Run single test | `ctest -R <test_name> --output-on-failure` |
| Run benchmarks | `cmake --build build --target run_benchmarks` (needs `-DHORCRUX_BUILD_BENCHMARKS=ON`) |
| Format C++ | `find . -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.cc' \| xargs clang-format-18 -i` |
| Pre-build hooks | `lefthook run pre-commit` / `lefthook run pre-push` |
| Skip hooks (emergency) | `LEFTHOOK=0 git commit` |

## Build targets

- `horcrux` — standalone executable (in `build/bin/`)
- `horcrux-cli` — CLI tool (output name `horcrux-cli`, in `build/bin/`)
- `horcrux_core` — static library
- `horcrux_cli` — static library

## Project structure

```
src/core/     → horcrux_core lib (build graph, adapters, Android, plugins, sandbox)
src/cli/      → horcrux_cli lib + horcrux-cli executable (commands: build, test, clean, query, import, doctor, plugin, registry)
tests/        → ~35 GTest executables linked to horcrux_core or horcrux_cli
benchmarks/   → 3 benchmark executables (Google Benchmark)
examples/     → hello, simple, java, python, rust, android, jni-hello
cmake/toolchains/ → ci.cmake, asan.cmake, ubsan.cmake, tsan.cmake, local.cmake, x86-64*.cmake
```

## Dependencies (all fetched by CMake)

- **OpenSSL** (system, for SHA256) — `find_package(OpenSSL REQUIRED)`
- **tl::expected** (FetchContent, v1.1.0) — `std::expected` polyfill
- **tinyxml2** (FetchContent, 10.0.0) — XML parsing for Android manifest merger
- **Google Test** (FetchContent, v1.16.0) — test framework

## CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `HORCRUX_BUILD_TESTS` | ON | Build unit tests |
| `HORCRUX_BUILD_BENCHMARKS` | ON | Build benchmarks |
| `HORCRUX_BUILD_EXAMPLES` | ON | Build example executables |

## CI & conventions

- **Default branch**: `production` (not `main`)
- **Versioning**: CalVer `YYYY.MMDD.PATCH` (e.g. `2026.0401.0`)
- **CI matrix**: GCC 14 + Clang 18 on ubuntu-24.04
- **Sanitizer gate** on release: ASan + UBSan (toolchains at `cmake/toolchains/`)
- **Commits**: conventional commits (`feat`, `fix`, `docs`, `refactor`, `perf`, `test`, `chore`)
- **Release**: tag `vYYYY.MMDD.PATCH` triggers artifact build + GitHub Release

## Code style (executable sources of truth)

- **`.clang-format`**: LLVM-based, IndentWidth: **2** (not 4), ColumnLimit: 100, K&R braces
- **Naming**: `snake_case` (vars/funcs), `PascalCase` (types), `SCREAMING_SNAKE_CASE` (constants/macros)
- **Error handling**: `std::expected` over exceptions in hot paths
- **Memory**: zero UB, RAII, `std::unique_ptr`/`shared_ptr`, no raw `new`/`delete`
- **Thread safety**: lock-free preferred, `std::scoped_lock` for mutexes, `std::atomic` for counters

## Architecture notes

- **Hexagonal (Ports & Adapters)**: domain layer is pure C++23 (no I/O), outer layers inject concrete adapters
- **Build graph**: immutable DAG built via `BuildGraph::builder()` pattern — construction single-threaded, reads thread-safe
- **Language adapters**: C++, Rust, Python, Java — all implement a common `Adapter` interface

## Existing instruction files

- `.github/copilot-instructions.md` — comprehensive C++23 + architecture guide
- `.github/agents/cpp-quality-agent.md` — dedicated C++ quality agent definition

## Testing quirks

- Tests link against `horcrux_core` (most) or `horcrux_cli + horcrux_core` (CLI tests)
- `import_command_test` and `cli_commands_test` and `plugin_command_test` link `horcrux_cli`
- Use `HORCRUX_BUILD_TESTS=OFF` to skip tests for faster compilation
- Run benchmarks with `cmake -B build -DHORCRUX_BUILD_BENCHMARKS=ON && cmake --build build && cd build && cmake --build . --target run_benchmarks`
- **Parallel flakiness**: ~30 test fixtures use hardcoded `/tmp/horcrux_*` temp dirs. Running `ctest -j$(nproc)` can cause spurious failures from shared state; `ctest -j1` resolves them.
- **Environment gating**: Java, Python, Rust integration tests auto-skip via `GTEST_SKIP()` if the compiler isn't on PATH.
- **Stale binaries**: Rebuild (`cmake --build build`) before running tests — stale test binaries can produce false failures.
