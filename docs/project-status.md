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
- Adapter architecture (M3.1):
  - Abstract `Adapter` interface with capability metadata, action planning, and cache key contracts.
  - `AdapterRegistry` for runtime adapter discovery and kind-based lookup.
  - `CppAdapter` MVP supporting `cc_library`, `cc_binary`, and `cc_test` target kinds.
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
- **Adapter architecture:** Foundation established with C++ MVP adapter.
- **End-to-end product completeness:** M3.1 complete; M3.2+ in progress.

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
