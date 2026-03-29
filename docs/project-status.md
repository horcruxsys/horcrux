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
  - `build`, `import`, and `doctor` commands are wired in the CLI entry point.
- Android pipeline components:
  - Toolchain detection, Java/Kotlin/Compose/DEX compilation helpers,
    manifest merger, resources processing, APK/AAB packaging, NDK support,
    sandboxing, and variant handling.
- Gradle interoperability:
  - Gradle parsing and workspace configuration generation.
- Test coverage:
  - Unit/integration test targets exist for core graph, cache, import flow,
    and all major Android modules listed above.

## Current Maturity Snapshot

- **Core build foundations:** Implemented and tested.
- **Android build modules:** Broad module coverage with dedicated tests.
- **CLI UX:** Usable for alpha workflows (`build`, `import`, `doctor`).
- **End-to-end product completeness:** In progress.

## Known Gaps / In-Progress Areas

From the current CLI and roadmap documents:

- `test`, `clean`, and `query` commands are listed but not fully implemented in CLI flow.
- Public documentation and operational guidance are spread across many files and still evolving.
- Broader language adapters and plugin/registry ecosystem are roadmap items.

## Recommended Near-Term Priorities

1. Finish CLI command completeness (`test`, `clean`, `query`) to match help output.
2. Add a single end-to-end build walkthrough that links architecture to real commands.
3. Continue consolidating docs around this status page and `docs/README.md` as canonical entry points.
4. Keep this page updated with dated milestone progress as CLI and adapters mature.

## Source of Truth

For current behavior, treat code and tests as authoritative:

- `src/core/`
- `src/cli/`
- `tests/`
