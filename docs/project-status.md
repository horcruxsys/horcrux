# Horcrux Project Status

**Last Updated:** 2026-04-01
**Status:** v2026.0401.0 — Stable Release

## What Horcrux Is Building

Horcrux is a universal build system written in C++23 with three primary goals:

1. Correctness-first, deterministic, hermetic builds.
2. High performance through immutable build graphs, caching, and parallel execution.
3. A developer-friendly CLI and migration path for existing projects (including Android/Gradle).

## What Is Implemented Today

Based on the current codebase and tests, the following areas are implemented:

- Plugin and Registry ecosystem (M5):
  - `PluginManifest` schema with parser/validator, SemVer versioning, permission model.
  - `PluginLoader` with lifecycle hooks (discover/initialize/shutdown) and deterministic alphabetical init order.
  - `PluginVerifier` for SHA-256 checksum verification and permission enforcement.
  - `RegistryClient` with in-memory package index, search, install/update/remove, and lockfile.
  - `plugin` CLI command: search/install/list/info/update/remove subcommands.
  - `registry` CLI command: add/remove/list subcommands.
- Core graph model:
  - `BuildNode`, `BuildEdge`, and immutable `BuildGraph` primitives.
- Caching:
  - Local cache implementation and tests.
  - Policy-fingerprinted cache keys (hermetic policy fingerprint mixed into every cache key).
- CLI surface:
  - `build`, `import`, `doctor`, `test`, `clean`, `query`, `plugin`, and `registry` commands are wired in the CLI entry point.
  - `--hermetic`, `--sandbox=<mode>`, and `--repro-check` flags on the `build` command.
- Hermetic sandboxing (M4):
  - `SandboxPolicy` model: modes (off/balanced/strict), network policy, env-var allowlist, path allowlist, resource limits, stable fingerprint.
  - `HermeticEnv` contract: normalized locale/TZ/tmpdir, deterministic path mapping, canonical input ordering (sort_inputs, sort_includes, sort_deps).
  - `ReproChecker`: double-build reproducibility verification, artifact hash comparison, non-determinism diagnostic hints.
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
  - M4: `sandbox_policy_test` (28 tests), `hermetic_env_test` (20 tests), `repro_checker_test` (21 tests).

## Current Maturity Snapshot

- **Core build foundations:** Implemented and tested.
- **Android build modules:** Broad module coverage with dedicated tests.
- **CLI UX:** Full command surface for alpha workflows (`build`, `import`, `doctor`, `test`, `clean`, `query`).
- **Adapter architecture:** Foundation (M3.1) + Rust/Python/Java adapters (M3.2) complete.
- **Hermetic sandboxing:** M4 core policy, environment contract, repro-check, and cache integration complete.
- **End-to-end product completeness:** M4 core complete.

## M6 Completion — 2026-04-01

### Completed

- [x] Version updated from `0.1.0` to `2026.0401.0` in `CMakeLists.txt`.
- [x] `CHANGELOG.md` created with full release history.
- [x] Release governance: `docs/release-process.md` — CalVer policy, branch strategy, freeze windows, blocker rubric, go/no-go criteria.
- [x] Release checklist: `docs/release-checklist.md` — signed-off gate covering CI, sanitizers, smoke tests, benchmarks, docs, and publication.
- [x] Expanded regression suites for critical paths:
  - `graph_regression_test` (graph resolution: transitive deps, cycle detection, topo order, diamond dedup, large graphs, stable hashes).
  - `cache_regression_test` (cache correctness: round-trip, miss, collision-free, overwrite, policy fingerprint mixing, large artifacts).
  - `sandbox_regression_test` (sandbox enforcement: mode round-trips, fingerprint determinism, policy-change differentiation, equality operators).
  - `adapter_regression_test` (adapter conformance: info, kind routing, foreign rejection, deterministic cache keys, action planning, diagnostic clearing, registry routing).
  - `plugin_lifecycle_regression_test` (plugin lifecycle: register, init, shutdown, version compat, permission gates, registry client round-trips).
- [x] Sanitizer toolchain files: `cmake/toolchains/asan.cmake`, `cmake/toolchains/ubsan.cmake`, `cmake/toolchains/tsan.cmake`.
- [x] Release workflow: `.github/workflows/release.yml` — triggered on version tags, builds multi-platform artifacts, runs sanitizer gate, publishes GitHub Release with checksums.
- [x] `scripts/package-release.sh` — reproducible local release packaging with SHA-256 checksums.
- [x] `docs/quick-start.md` — installation, first build, hermetic builds, plugin management, next-steps navigation.
- [x] `docs/migration-guide.md` — migration from CMake/Ninja, Bazel, Buck2, Gradle, and alpha.
- [x] `docs/troubleshooting.md` — general troubleshooting playbook (install, build, cache, plugins, tests, Gradle, doctor).
- [x] `docs/benchmark-methodology.md` — reference hardware, workload definitions (WL-1 to WL-8), measurement procedure, baseline comparisons, regression threshold policy.
- [x] `docs/README.md` updated with new documentation entries.
- [x] `docs/project-status.md` updated with M6 completion entry (this entry).
- [x] `README.md` roadmap updated: all milestones M1–M6 marked complete.

### Known Gaps for v2026.07xx+

- Real HTTP registry client (M5 uses in-memory index).
- Dynamic shared-library plugin loading (M5 is in-process only).
- BUILD file parsing (replacing demo workspace graphs).
- Real compiler invocation via adapters (currently plans actions only).
- Remote/distributed caching.
- Linux namespace isolation for `SandboxMode::Strict` on general (non-Android) actions.
- `SOURCE_DATE_EPOCH` timestamp normalisation.
- IDE integrations.

## M5 Completion — 2026-03-30

### Completed

- [x] `PluginManifest` schema: name, version (SemVer), author, license, description, min/max Horcrux version, permissions, extension points.
- [x] `parse_plugin_manifest` parser: line-oriented TOML-like format, `[extension]` blocks, permission flags, optional checksum.
- [x] `validate_plugin_manifest`: compatibility range check, extension kind validation.
- [x] `PluginTrustPolicy`: default (fs_read/write/process_spawn, no network) and strict (fs_read/write only) policies.
- [x] `verify_plugin_checksum`: SHA-256 checksum verification of plugin binary against manifest.
- [x] `enforce_plugin_permissions`: gates permission declarations against active trust policy.
- [x] `compute_file_sha256`: reads a file and returns its SHA-256 hash.
- [x] `PluginLoader`: register/scan/initialize/shutdown lifecycle; deterministic alphabetical init order; fault isolation per plugin; compatibility and permission gates.
- [x] `scan_directory`: discovers plugins in a directory tree by reading `manifest.toml` files.
- [x] `RegistryClient`: in-memory package index; search (case-insensitive substring); package_info; install/update/remove with lockfile update.
- [x] `PluginLockfile`: records exact installed plugin set with name, version, checksum, registry URL; roundtrip read/write.
- [x] `RegistryConfig`: named registry entries with URL and trusted flag; persistent config file.
- [x] `plugin` CLI command: `search`, `install`, `list`, `info`, `update`, `remove` subcommands with `--plugins-dir` and `--lockfile` options.
- [x] `registry` CLI command: `add`, `remove`, `list` subcommands with `--config` option and `--trusted` flag.
- [x] `main.cpp` wired with `plugin` and `registry` commands; help text updated.
- [x] Unit tests: `plugin_manifest_test` (18), `plugin_loader_test` (13), `registry_client_test` (20), `plugin_command_test` (20) — 71 new tests.
- [x] `tests/CMakeLists.txt` updated with 4 new test targets.
- [x] `src/core/CMakeLists.txt` and `src/cli/CMakeLists.txt` updated with new source files.
- [x] `docs/plugin-api.md` — Plugin API reference (manifest schema, lifecycle, versioning, permissions, extension points).
- [x] `docs/registry-guide.md` — Registry and plugin command user guide.
- [x] `docs/plugin-authoring.md` — Plugin authoring tutorial with manifest checklist, SDK skeleton, compatibility testing.
- [x] `docs/project-status.md` updated with M5 completion entry.

### Known Gaps for M5.1+

- Real HTTP registry client (M5 uses in-memory index; network fetch is a future milestone).
- Dynamic shared-library plugin loading (M5 is in-process only).
- Per-plugin rule integration with BUILD file parser.
- Marketplace UI beyond CLI.
- Runtime remote execution plugins.
- Enterprise policy management portal.

## M4 Completion — 2026-03-30

### Completed

- [x] `SandboxPolicy` model: `SandboxMode` (off/balanced/strict), `NetworkPolicy` (deny/allowlist/allow), env-var allowlist, host-path allowlist, resource limits, stable SHA-256 fingerprint, equality operators.
- [x] `sandbox_mode_from_string` / `to_string` converters for CLI flag parsing.
- [x] `HermeticEnv` deterministic environment contract: filters env vars to allowlist, normalizes locale/TZ/tmpdir/output-root, canonical `sort_inputs` / `sort_includes` / `sort_deps` helpers.
- [x] `ReproChecker`: double-build artifact hash capture, comparison, `ReproReport` with `differing_artifacts()`, `non_hermetic_hints()`, `summary()`.
- [x] `hash_file` utility and `compare_artifacts` helper.
- [x] `mix_policy_fingerprint` in `local_cache`: combines base cache key with policy fingerprint so cache hits are invalidated on policy change.
- [x] `BuildExecutor::create_with_policy` factory accepting `SandboxPolicy` + `repro_check` flag.
- [x] `BuildError::PolicyError` and `BuildError::ReproCheckFailed` error codes.
- [x] `build` command in `main.cpp` wired with `--hermetic`, `--sandbox=<mode>`, `--repro-check` flags.
- [x] Policy fingerprint mixed into cache keys in `execute_build`.
- [x] Sandbox mode logged during build execution.
- [x] Unit tests: `sandbox_policy_test` (28), `hermetic_env_test` (20), `repro_checker_test` (21) — 69 new tests.
- [x] `tests/CMakeLists.txt` updated with new test targets.
- [x] `src/core/CMakeLists.txt` updated with new source files.
- [x] `docs/hermetic-build.md` — user guide for hermetic builds, CLI flags, env contract, repro-check, cache fingerprinting, migration guide.
- [x] `docs/sandbox-troubleshooting.md` — troubleshooting guide for sandbox failures, repro-check failures, common causes and fixes.
- [x] `docs/project-status.md` updated with M4 completion entry.

### Known Gaps for M4.1+

- Linux namespace-based isolation (`SandboxMode::Strict`) is modelled but not yet wired to the OS-level `AndroidSandbox` for general (non-Android) actions.
- `--sandbox` per-rule overrides via `horcrux.yaml` are modelled but BUILD file parsing is not yet implemented.
- Network allowlist rules are modelled but not enforced at the kernel level.
- `SOURCE_DATE_EPOCH` support for timestamp normalization.

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
3. Wire `SandboxMode::Strict` to the OS-level namespace sandbox for general actions.
4. Keep this page updated with dated milestone progress as CLI and adapters mature.

## Source of Truth

For current behavior, treat code and tests as authoritative:

- `src/core/`
- `src/cli/`
- `tests/`

