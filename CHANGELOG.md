# Changelog

All notable changes to Horcrux are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Horcrux uses **CalVer** versioning: `YYYY.MMDD.PATCH` (e.g., `2026.0401.0`).

---

## [2026.0401.0] — 2026-04-01

### Summary

This is the first stable release of Horcrux — `v2026.0401.0`. It delivers a
complete, production-ready build system with hermetic sandboxing, a plugin and
registry ecosystem, multi-language adapters, and reproducible builds.

### Added

#### Core Engine (M1–M2)
- Immutable `BuildNode`, `BuildEdge`, and `BuildGraph` DAG primitives.
- `LocalCache` with content-addressed lookup and policy fingerprinting.
- Parallel task scheduler with dependency-ordered execution.

#### CLI (M2.5–M3.1)
- `build` — compile and link targets with progress reporting.
- `import` — Gradle project import with workspace config generation.
- `doctor` — environment validation and dependency health checks.
- `test` — test target execution with pass/fail/skip summary.
- `clean` — scoped output and cache removal (`--all`, `--outputs`, `--cache`, `--dry-run`).
- `query` — dependency graph inspection (deps, rdeps, topo, all targets) with text/JSON output.
- `plugin` — plugin lifecycle management (search/install/list/info/update/remove).
- `registry` — registry management (add/remove/list).

#### Language Adapters (M3.1–M3.2)
- Abstract `Adapter` interface with capability metadata, action planning, and cache-key contracts.
- `AdapterRegistry` for runtime adapter discovery and kind-based routing.
- `CppAdapter` — `cc_library`, `cc_binary`, `cc_test`.
- `RustAdapter` — `rust_library`, `rust_binary`, `rust_test`.
- `PythonAdapter` — `py_library`, `py_binary`, `py_test`.
- `JavaAdapter` (non-Android) — `java_library`, `java_binary`, `java_test`.
- Cross-adapter conformance suite (10 contracts across all 4 adapters).

#### Android Build Pipeline (M3.x)
- Toolchain detection and variant handling.
- Java, Kotlin, Compose, DEX, NDK compilation helpers.
- Manifest merger, resource processing, APK/AAB packaging.
- Android-specific sandboxing.

#### Hermetic Builds & Sandboxing (M4)
- `SandboxPolicy` — `SandboxMode` (off/balanced/strict), `NetworkPolicy`, env-var
  and path allowlists, resource limits, stable SHA-256 fingerprint.
- `HermeticEnv` — normalized locale/TZ/tmpdir, deterministic input ordering
  (`sort_inputs`, `sort_includes`, `sort_deps`).
- `ReproChecker` — double-build reproducibility verification, artifact hash
  comparison, non-determinism diagnostic hints.
- `--hermetic`, `--sandbox=<mode>`, `--repro-check` build flags.
- Policy fingerprint mixed into every cache key.

#### Plugin & Registry System (M5)
- `PluginManifest` — TOML-like schema with SemVer versioning, permissions,
  extension points, and optional checksum.
- `PluginLoader` — lifecycle hooks (discover/initialize/shutdown), deterministic
  alphabetical init order, per-plugin fault isolation.
- `PluginVerifier` — SHA-256 checksum verification and permission enforcement.
- `RegistryClient` — package index, search, install/update/remove, lockfile.

#### Release Readiness (M6)
- Stable `v2026.0401.0` release tag with verified artifacts.
- Expanded regression suites for all critical paths:
  - Graph resolution (`graph_regression_test`).
  - Cache correctness (`cache_regression_test`).
  - Sandbox enforcement (`sandbox_regression_test`).
  - Adapter conformance (`adapter_regression_test`).
  - Plugin lifecycle (`plugin_lifecycle_regression_test`).
- Sanitizer and static-analysis gates on release candidates (AddressSanitizer,
  UndefinedBehaviorSanitizer, ThreadSanitizer via `cmake/toolchains/asan.cmake`
  and `cmake/toolchains/ubsan.cmake`).
- Release workflow (`.github/workflows/release.yml`) with checksummed artifact
  publication.
- `scripts/package-release.sh` — reproducible release artifact packaging.
- Documentation: quick-start, migration guide, troubleshooting playbook,
  benchmark methodology, release process, and release checklist.
- `CHANGELOG.md` (this file).

### Changed
- CMakeLists.txt version updated from `0.1.0` to `2026.0401.0`.
- README roadmap updated: all milestones M1–M6 marked complete.
- `docs/project-status.md` updated with M6 completion entry.
- `docs/README.md` index updated with new documentation entries.

### Infrastructure
- GitHub Actions release workflow with signed artifact publication.
- Benchmark CI (`benchmarks.yml`) tracking cold/warm/incremental rebuild times.
- AddressSanitizer and UBSan toolchain files for release-candidate gating.

---

## [0.1.0-alpha] — 2026-03-30

Pre-release alpha covering milestones M1 through M5. See
`docs/project-status.md` for a complete per-milestone breakdown of what was
implemented.

---

[2026.0401.0]: https://github.com/horcruxsys/horcrux/releases/tag/v2026.0401.0
[0.1.0-alpha]: https://github.com/horcruxsys/horcrux/releases/tag/v0.1.0-alpha
