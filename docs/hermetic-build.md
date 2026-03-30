# Hermetic Builds in Horcrux

**Last Updated:** 2026-03-30
**Status:** Implemented (M4)

## What Is a Hermetic Build?

A hermetic build is one that produces the **same outputs regardless of the machine, user environment, or host configuration** on which it is run. All inputs must be explicitly declared; no hidden dependencies are allowed.

Horcrux enforces hermeticity via:

1. **Sandbox Policy** — controls which env vars, host paths, and network calls an action is allowed to use.
2. **Deterministic Environment Contract** — normalizes locale, timezone, and temp-directory paths.
3. **Policy-Fingerprinted Cache Keys** — cache hits are only valid when the policy that produced an artifact matches the current policy.
4. **Reproducibility Check** (`--repro-check`) — runs two independent builds and compares output hashes.

---

## Sandbox Modes

Use `--sandbox=<mode>` to control isolation level:

| Mode | Description |
|------|-------------|
| `off` | No sandboxing. All host env vars and paths are accessible. Useful for debugging. |
| `balanced` | *(Default)* Env-var filtering and path allowlists. No kernel namespace isolation. |
| `strict` | Full isolation using Linux namespaces in addition to all Balanced controls. |

Shorthand: `--hermetic` is equivalent to `--sandbox=balanced`.

---

## CLI Flags

```
--hermetic           Enable balanced hermetic sandboxing (default for build)
--sandbox=MODE       Set sandbox mode: strict | balanced | off
--repro-check        Run the double-build reproducibility check after a successful build
```

### Examples

```bash
# Build with default hermetic settings (balanced)
horcrux build //examples/hello:app

# Build with strict isolation
horcrux build //examples/hello:app --sandbox=strict

# Build without any sandboxing (debugging/migration)
horcrux build //examples/hello:app --sandbox=off

# Run repro-check to verify the build is reproducible
horcrux build //examples/hello:app --repro-check
```

---

## Environment Contract

When sandbox mode is `balanced` or `strict`, Horcrux injects the following normalized values into every action's environment, overriding any host values:

| Variable | Normalized Value |
|----------|-----------------|
| `LANG`, `LC_ALL`, `LC_CTYPE` | `en_US.UTF-8` |
| `TZ` | `UTC` |
| `TMPDIR`, `TMP`, `TEMP` | `/tmp/horcrux-sandbox` |
| `HORCRUX_OUTPUT_ROOT` | Build output root path |

Only variables listed in the **env-var allowlist** are passed through from the host. The default allowlist includes:

- `HOME`, `PATH`, `USER`, `LOGNAME`
- `LANG`, `LC_ALL`, `LC_CTYPE`, `TZ`
- `TMPDIR`, `TMP`, `TEMP`
- `HORCRUX_CACHE_DIR`, `HORCRUX_OUTPUT_ROOT`

Any variable not in the allowlist is stripped. Use `--sandbox=off` if you need unrestricted access during migration.

---

## Canonical Input Ordering

To prevent subtle non-determinism from ordering differences, Horcrux normalizes all input lists before passing them to adapters:

- **Source files** — sorted lexicographically, duplicates removed.
- **Include / classpath entries** — sorted lexicographically, duplicates removed.
- **Dependency closure** — sorted lexicographically by label, duplicates removed.

---

## Reproducibility Check (`--repro-check`)

When `--repro-check` is passed, Horcrux:

1. Runs build round 1.
2. Runs build round 2 (same inputs, same policy).
3. Computes SHA-256 hashes of all output artifacts.
4. Compares the two sets of hashes.

If all hashes match, the build is **reproducible** (PASS). If any hash differs, the build is **non-reproducible** (FAIL) and a diagnostic report is printed, including:

- A list of differing artifacts.
- Hints about common non-determinism causes (embedded timestamps, PID-based filenames, etc.).
- Suggested mitigations (e.g., `SOURCE_DATE_EPOCH`, `-ffile-prefix-map`).

### Example output

```
Reproducibility check: PASS
  Artifacts compared: 3
  Artifacts matched:  3
  Artifacts differed: 0
```

```
Reproducibility check: FAIL
  Artifacts compared: 2
  Artifacts matched:  1
  Artifacts differed: 1

Differing artifacts:
  - app.debug

Diagnostics:
  Some build artifacts differed between rounds, suggesting non-hermetic inputs.
  Hint: Embedded timestamps detected in debug/dependency files. Consider passing
        -ffile-prefix-map or SOURCE_DATE_EPOCH.
  Hint: Run with --sandbox=strict and check for undeclared host tool or env-var access.
```

---

## Policy-Fingerprinted Cache Keys

Cache hit validity is tied to the sandbox policy that produced an artifact. Internally, every cache key is:

```
cache_key = SHA-256(base_key || policy_fingerprint)
```

Where `policy_fingerprint` is a deterministic SHA-256 hash of the entire `SandboxPolicy` struct (mode, network policy, env allowlist, path allowlist, resource limits, etc.).

This means:

- Switching from `--sandbox=balanced` to `--sandbox=strict` automatically invalidates all existing cache entries.
- Switching from `--sandbox=balanced` to `--sandbox=off` similarly invalidates the cache.
- Same policy always reuses cached artifacts.

---

## Migration Guide

### Projects with implicit host dependencies

If your project currently relies on host tools, env vars, or paths that are not declared:

1. Start with `--sandbox=off` to establish a baseline.
2. Switch to `--sandbox=balanced`. Build failures or new diagnostics indicate undeclared dependencies.
3. Add the required env vars to your build rule metadata.
4. Switch to `--sandbox=strict` once all dependencies are declared.

### Projects that embed build timestamps

Set the `SOURCE_DATE_EPOCH` environment variable (and add it to the allowlist) to produce reproducible timestamps:

```bash
export SOURCE_DATE_EPOCH=$(git log -1 --format=%ct)
horcrux build //my:target --repro-check
```

---

## Network Access Policy

By default (`--sandbox=balanced` and `--sandbox=strict`), all network access is denied. To allow specific hosts, configure `allowed_network_rules` in your build configuration (future: `horcrux.yaml`). Use `--sandbox=off` to disable network restrictions entirely.

---

## See Also

- [Sandbox Troubleshooting](sandbox-troubleshooting.md)
- [Architecture](architecture.md)
- [Project Status](project-status.md)
