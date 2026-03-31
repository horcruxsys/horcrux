# Troubleshooting

This guide covers common issues with Horcrux and how to resolve them. For
sandbox-specific issues see [sandbox-troubleshooting.md](sandbox-troubleshooting.md).

## Table of Contents

- [Installation Issues](#installation-issues)
- [Build Failures](#build-failures)
- [Cache Issues](#cache-issues)
- [Plugin Issues](#plugin-issues)
- [Test Failures](#test-failures)
- [Gradle Import Issues](#gradle-import-issues)
- [Environment Validation](#environment-validation)
- [Getting Help](#getting-help)

---

## Installation Issues

### `horcrux: command not found`

**Cause:** The Horcrux binary is not on your `PATH`.

**Fix:**

```bash
# Check where it was installed
which horcrux || find /usr /opt ~/.local -name horcrux 2>/dev/null

# Add to PATH (bash/zsh)
export PATH="/usr/local/bin:$PATH"

# For a permanent fix, add the export to ~/.bashrc or ~/.zshrc
```

### `horcrux --version` reports wrong version

**Cause:** Multiple Horcrux installations exist and the wrong one is on `PATH`.

**Fix:**

```bash
which -a horcrux       # list all horcrux binaries on PATH
hash -r                # clear shell command cache (bash)
```

### OpenSSL not found at runtime

**Cause:** Horcrux is dynamically linked against OpenSSL and the library is
not found at runtime.

**Fix (Linux):**

```bash
ldd "$(which horcrux)" | grep ssl   # verify expected path
sudo ldconfig                        # refresh linker cache
```

**Fix (macOS):**

```bash
DYLD_LIBRARY_PATH=/opt/homebrew/opt/openssl@3/lib horcrux --version
```

---

## Build Failures

### `BuildError: target not found`

**Cause:** The label passed to `horcrux build` does not exist in the workspace.

**Fix:**

```bash
# List all available targets
horcrux query //...

# Check for typos in the label
horcrux build //src:my_binary   # correct
horcrux build //src:myBinary    # labels are case-sensitive
```

### Build exits non-zero with no error message

**Cause:** The adapter for the target language could not find the required
toolchain (compiler, linker, etc.).

**Fix:**

```bash
# Run doctor to validate the environment
horcrux doctor

# Check toolchain overrides
export HORCRUX_CXX=/usr/bin/clang++-18
export HORCRUX_RUSTC=/usr/local/bin/rustc
export HORCRUX_PYTHON=/usr/bin/python3
export HORCRUX_JAVAC=/usr/lib/jvm/java-21/bin/javac
```

### `PolicyError` during build

**Cause:** A `--sandbox` or `--hermetic` policy constraint was violated.

**Fix:** See [sandbox-troubleshooting.md](sandbox-troubleshooting.md).

### Circular dependency error

**Cause:** The build graph contains a cycle.

**Fix:**

```bash
# Identify the cycle
horcrux query --cycles //...
```

Review the reported dependency chain and remove the circular reference from
your target definitions.

---

## Cache Issues

### Cache miss on every build

**Cause:** Common causes include:
1. `--sandbox` policy changed, invalidating fingerprinted cache keys.
2. Input files have non-deterministic content (e.g., embedded timestamps).
3. The cache directory was deleted.

**Diagnosis:**

```bash
# Check the cache directory
ls -lh ~/.horcrux/cache/ 2>/dev/null || echo "Cache directory not found"

# Run a reproducibility check to detect non-determinism
horcrux build --repro-check //...
```

**Fix:**

- If the policy changed intentionally, the cache miss is expected behaviour.
- If inputs contain timestamps, normalise them or use `SOURCE_DATE_EPOCH`.
- If the cache directory is missing, it will be recreated automatically.

### Stale cache artifact used after source change

**Cause:** The cache key computation did not include a changed input file.

**Fix:**

```bash
# Force a clean rebuild
horcrux clean --cache
horcrux build //...
```

If the issue recurs, file a bug with the target label and the changed files.

### Cache directory is too large

```bash
# Show cache size
du -sh ~/.horcrux/cache/

# Remove only cached artifacts (keeps build outputs)
horcrux clean --cache
```

---

## Plugin Issues

### `plugin install` fails with `checksum mismatch`

**Cause:** The downloaded plugin binary does not match the checksum in the
manifest. This may indicate a corrupted download or a tampered artifact.

**Fix:**

```bash
# Retry the install (re-downloads the plugin)
horcrux plugin install --force my-plugin

# If the issue persists, contact the plugin author or check the registry
horcrux registry list
```

### Plugin not found after install

**Cause:** The plugins directory differs between install and runtime.

**Fix:**

```bash
# Specify the plugins directory explicitly
horcrux plugin list --plugins-dir ~/.horcrux/plugins
horcrux build --plugins-dir ~/.horcrux/plugins //...
```

### Plugin compatibility error

**Cause:** The installed plugin requires a different Horcrux version range.

**Fix:**

```bash
# Check plugin info
horcrux plugin info my-plugin

# Check current Horcrux version
horcrux --version
```

Update either the plugin or Horcrux to a compatible version.

---

## Test Failures

### `horcrux test` reports failures but the code looks correct

**Cause:** Test isolation is not guaranteed without sandbox mode. A test may
be passing or failing due to environment state from a prior run.

**Fix:**

```bash
# Run tests with strict isolation
horcrux test --sandbox=strict //...

# Check for test output files
horcrux clean --outputs
horcrux test //...
```

### `horcrux test` exits with `skipped: 0, failed: 0, passed: 0`

**Cause:** No test targets match the label pattern.

**Fix:**

```bash
# Check that test targets exist
horcrux query //... | grep _test
```

---

## Gradle Import Issues

### `horcrux import` fails with `Gradle project not found`

**Cause:** The specified directory does not contain a `build.gradle` or
`settings.gradle` file.

**Fix:**

```bash
ls /path/to/project/*.gradle*   # verify Gradle files exist
horcrux import /path/to/project
```

### Imported workspace has missing targets

**Cause:** Some Gradle modules may not map to Horcrux target kinds yet.

**Fix:** Review the generated `horcrux.yaml` and add missing targets manually.
See [gradle-interop.md](gradle-interop.md) for the full mapping reference.

---

## Environment Validation

Run `horcrux doctor` to perform a comprehensive environment check:

```bash
horcrux doctor
```

The doctor command checks:
- Required tools (`cmake`, compiler, `ninja`, `openssl`).
- Plugin directory accessibility.
- Cache directory permissions.
- `PATH` sanity.
- Toolchain overrides (env vars).

### Common doctor warnings

| Warning | Meaning | Fix |
|---------|---------|-----|
| `OPENSSL_ROOT_DIR not set` | OpenSSL path not configured | `export OPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3` (macOS) |
| `compiler not found` | C++ compiler is missing or not on PATH | Install GCC 13+ or Clang 17+ |
| `cache directory not writable` | Permissions issue on cache dir | `chmod 700 ~/.horcrux/cache` |
| `ninja not found` | Ninja build tool missing | Install ninja-build |

---

## Getting Help

1. **Documentation:** Browse the full docs at [docs/README.md](README.md).
2. **GitHub Discussions:** Post questions at
   [github.com/horcruxsys/horcrux/discussions](https://github.com/horcruxsys/horcrux/discussions).
3. **Issue Tracker:** Report confirmed bugs at
   [github.com/horcruxsys/horcrux/issues](https://github.com/horcruxsys/horcrux/issues).

When filing a bug, always include:
- Horcrux version (`horcrux --version`).
- Operating system and version.
- Compiler name and version.
- The full command you ran and the complete output.
- The output of `horcrux doctor`.
