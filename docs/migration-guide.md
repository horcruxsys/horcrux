# Migration Guide — Horcrux v2026.0401.0

This guide helps you migrate to Horcrux from other build systems or from a
pre-release alpha version.

## Table of Contents

- [Migrating from Alpha (pre-v2026.0401.0)](#migrating-from-alpha)
- [Migrating from CMake / Ninja](#from-cmake--ninja)
- [Migrating from Bazel](#from-bazel)
- [Migrating from Buck2](#from-buck2)
- [Migrating from Gradle (Android/Java)](#from-gradle)

---

## Migrating from Alpha

If you were using a pre-release Horcrux alpha, here is what changed in
`v2026.0401.0`:

### Version string

The `--version` output now uses CalVer format:

```
# Before (alpha)
Horcrux 0.1.0

# After (v2026.0401.0)
Horcrux 2026.0401.0
```

### CMake project version

If you embed Horcrux as a CMake sub-project, update your `find_package` call:

```cmake
# Before
find_package(Horcrux 0.1 REQUIRED)

# After
find_package(Horcrux 2026.0401 REQUIRED)
```

### Plugin manifest compatibility

The `min_horcrux_version` / `max_horcrux_version` fields in `manifest.toml`
use the same CalVer scheme. Update any pinned range:

```toml
# Before
min_horcrux_version = "0.1.0"

# After
min_horcrux_version = "2026.0401.0"
```

### No breaking API changes

The public CLI surface (`build`, `test`, `clean`, `query`, `import`, `doctor`,
`plugin`, `registry`) is unchanged from the last alpha. Existing scripts do not
require modification.

---

## From CMake / Ninja

### Concept mapping

| CMake | Horcrux |
|-------|---------|
| `CMakeLists.txt` | `horcrux.yaml` |
| `add_library(foo STATIC ...)` | `cc_library(name="foo", srcs=[...])` |
| `add_executable(bar ...)` | `cc_binary(name="bar", deps=["//src:foo"])` |
| `add_test(NAME t COMMAND bar)` | `cc_test(name="t", srcs=[...])` |
| `cmake -B build && cmake --build build` | `horcrux build //...` |
| `ctest` | `horcrux test //...` |
| `cmake --install` | `horcrux build --install //...` |

### Step-by-step

1. Install Horcrux (see [quick-start.md](quick-start.md)).
2. Run `horcrux doctor` to validate your environment.
3. Create a `horcrux.yaml` at the root of your project describing your targets.
4. Run `horcrux build //...` — Horcrux will build in dependency order.
5. Run `horcrux test //...` to execute all test targets.

---

## From Bazel

### Concept mapping

| Bazel | Horcrux |
|-------|---------|
| `WORKSPACE` | `horcrux.yaml` workspace stanza |
| `BUILD` / `BUILD.bazel` | `horcrux.yaml` (per-package) |
| `//path/to:target` | `//path/to:target` (same syntax) |
| `bazel build //...` | `horcrux build //...` |
| `bazel test //...` | `horcrux test //...` |
| `bazel query ...` | `horcrux query ...` |
| `bazel clean` | `horcrux clean` |
| `cc_library`, `cc_binary` | `cc_library`, `cc_binary` (same names) |
| `rust_library`, `rust_binary` | `rust_library`, `rust_binary` |
| `py_library`, `py_binary` | `py_library`, `py_binary` |
| `java_library`, `java_binary` | `java_library`, `java_binary` |
| `.bazelrc` | CLI flags or `horcrux.yaml` options stanza |
| Remote execution | Planned (post-v1.0) |
| Starlark extensions | Horcrux plugin system (see [plugin-authoring.md](plugin-authoring.md)) |

### Key differences

- Horcrux does not yet support `BUILD` file parsing; target descriptions live
  in `horcrux.yaml` files. `BUILD` file parsing is on the roadmap.
- Bazel's `WORKSPACE` external dependency mechanism is replaced by the
  `horcrux plugin install` workflow.
- Remote execution and remote caching are planned for a future release.

---

## From Buck2

### Concept mapping

| Buck2 | Horcrux |
|-------|---------|
| `BUCK` files | `horcrux.yaml` |
| `//cell//path:target` | `//path:target` |
| `buck2 build` | `horcrux build` |
| `buck2 test` | `horcrux test` |
| `buck2 query` | `horcrux query` |
| `buck2 clean` | `horcrux clean` |
| Starlark rules | Horcrux plugin system |

---

## From Gradle

For Android and JVM projects, use the built-in Gradle importer:

```bash
# Import a Gradle project (reads build.gradle / settings.gradle)
horcrux import /path/to/gradle/project
```

This generates a `horcrux.yaml` workspace configuration from the Gradle project
structure. After import:

1. Review the generated `horcrux.yaml` for correctness.
2. Run `horcrux build //...` to verify the imported project builds.
3. Run `horcrux test //...` to validate tests.

### Android-specific notes

- Horcrux natively supports `debug` and `release` Android build variants.
- APK and AAB packaging use the same toolchain as your Gradle project.
- NDK targets are auto-detected via the `android.ndkVersion` property in
  `build.gradle`.

See [gradle-interop.md](gradle-interop.md) for full Gradle integration details.

---

## Rollback Guidance

If you need to revert to a previous Horcrux version:

```bash
# Check installed version
horcrux --version

# Download a previous release from GitHub
# https://github.com/horcruxsys/horcrux/releases

# Replace the binary
sudo cp horcrux-2026.0401.0-linux-x86_64 /usr/local/bin/horcrux

# Verify
horcrux --version
```

The `LocalCache` format is backward-compatible. Running an older binary against
a cache written by a newer version will safely miss all entries (keys are
version-independent content-addressed hashes). No cache corruption will occur.
