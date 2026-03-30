# Horcrux Project Status

**Last Updated:** 2026-03-29
**Status:** Active alpha development

## What Horcrux Is Building

Horcrux is a universal build system written in C++23 with three primary goals:

1. Correctness-first, deterministic, hermetic builds.
2. High performance through immutable build graphs, caching, and parallel execution.
3. A developer-friendly CLI and migration path for existing projects (including Android/Gradle).

## What Is Implemented Today

Based on the current codebase and tests, the following areas are implemented:

- Core graph model:
  - `BuildNode`, `BuildEdge`, and immutable `BuildGraph` primitives.
- Caching:
  - Local cache implementation and tests.
- CLI surface:
  - `build`, `import`, `doctor`, `test`, `clean`, and `query` commands are wired in the CLI entry point.
- Adapter architecture (M3.1 + M3.2):
  - Abstract `Adapter` interface with capability metadata, action planning, and cache key contracts.
  - `AdapterRegistry` for runtime adapter discovery and kind-based lookup.
  - `CppAdapter` MVP supporting `cc_library`, `cc_binary`, and `cc_test` target kinds.
  - `RustAdapter` supporting `rust_library`, `rust_binary`, and `rust_test` target kinds.
  - `PythonAdapter` supporting `py_library`, `py_binary`, and `py_test` target kinds.
  - `JavaAdapter` (non-Android) supporting `java_library`, `java_binary`, and `java_test` target kinds.
- Android pipeline components:
  - Toolchain detection, Java/Kotlin/Compose/DEX compilation helpers,
    manifest merger, resources processing, APK/AAB packaging, NDK support,
    sandboxing, and variant handling.
- Gradle interoperability:
  - Gradle parsing and workspace configuration generation.
- Test coverage:
  - Unit/integration test targets exist for core graph, cache, import flow,
    all major Android modules listed above, CLI commands (test/clean/query),
    adapter interface, and C++ adapter.

## Current Maturity Snapshot

- **Core build foundations:** Implemented and tested.
- **Android build modules:** Broad module coverage with dedicated tests.
- **CLI UX:** Full command surface for alpha workflows (`build`, `import`, `doctor`, `test`, `clean`, `query`).
- **Adapter architecture:** Foundation (M3.1) + Rust/Python/Java adapters (M3.2) complete.
- **End-to-end product completeness:** M3 complete.

## M3.2 Completion — 2026-03-29

### Completed

- [x] `RustAdapter` MVP: `rust_library` (compile → rlib), `rust_binary` (compile), `rust_test` (compile + test run).
- [x] `PythonAdapter` MVP: `py_library` (py_compile packaging), `py_binary` (packaging), `py_test` (packaging + unittest run).
- [x] `JavaAdapter` MVP (non-Android): `java_library` (javac + jar), `java_binary` (javac + executable jar), `java_test` (javac + jar + test run).
- [x] Toolchain detection for each adapter with env-var override (`HORCRUX_RUSTC`, `HORCRUX_PYTHON`, `HORCRUX_JAVAC`).
- [x] Deterministic cache keys for all three adapters (label + kind + sources + flags + toolchain version).
- [x] Structured diagnostics emitted on unsupported kind and planning errors.
- [x] Cross-adapter conformance test suite (10 contracts verified across all 4 adapters).
- [x] Per-adapter unit tests: 22 Rust, 20 Python, 22 Java tests.
- [x] All adapters registered in `AdapterRegistry`; kind-based routing works across all 12 target kinds.
- [x] Workspace graph updated with Rust/Python/Java example targets.
- [x] Example projects added: `examples/rust/`, `examples/python/`, `examples/java/`.
- [x] Per-language README with toolchain setup, build/test/query commands, and layout.
- [x] `docs/project-status.md` updated with M3.2 completion entry.

### Known Gaps for M3.3+

- BUILD file parsing (currently uses demo workspace graph).
- Real compiler invocation via adapters (currently plans actions only; execution simulated).
- Distributed/remote caching.
- IDE integrations.

## M3.1 Completion — 2026-03-29

### Completed

- [x] `test` command: executes test targets, reports total/passed/failed/skipped summary, returns non-zero exit on failures.
- [x] `clean` command: removes build outputs and/or cache with scope flags (`--all`, `--outputs`, `--cache`), path safety validation, `--dry-run` support.
- [x] `query` command: queries direct deps, transitive deps, reverse deps, topological order, all targets; outputs as text or JSON.
- [x] All three commands wired in `main.cpp` with consistent option parsing.
- [x] `Adapter` abstract interface: capability metadata, `parse_target`, `plan_actions`, `compute_cache_key`, `diagnostics`.
- [x] `AdapterRegistry`: runtime registration, lookup by name, lookup by target kind.
- [x] `CppAdapter` MVP: `cc_library` (compile + archive), `cc_binary` (compile + link), `cc_test` (compile + link).
- [x] Deterministic cache key computation from normalized inputs.
- [x] Unit tests for test/clean/query CLI commands.
- [x] Unit tests for adapter interface contracts and CppAdapter.
- [x] Updated `docs/cli-implementation.md` to reflect all commands.

### Known Gaps for M3.2+

- BUILD file parsing (currently uses demo graphs).
- Real compiler invocation via CppAdapter (currently plans actions only).
- Rust/Python/Java adapter implementations.
- Distributed/remote caching.
- End-user IDE integrations.

## Known Gaps / In-Progress Areas

From the current CLI and roadmap documents:

- Public documentation and operational guidance are spread across many files and still evolving.
- Broader language adapters and plugin/registry ecosystem are roadmap items.
- `test` and `clean` commands have basic wiring; full execution requires BUILD file parsing.

## Recommended Near-Term Priorities

1. Add BUILD file parsing to replace demo graphs with real target resolution.
2. Invoke CppAdapter compile/link actions in BuildExecutor for real compilation.
3. Continue consolidating docs around this status page and `docs/README.md` as canonical entry points.
4. Create M3.2 issues for Rust, Python, and Java adapters.
5. Keep this page updated with dated milestone progress as CLI and adapters mature.

## Source of Truth

For current behavior, treat code and tests as authoritative:

- `src/core/`
- `src/cli/`
- `tests/`
