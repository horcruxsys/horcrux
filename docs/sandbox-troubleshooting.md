# Sandbox Troubleshooting Guide

**Last Updated:** 2026-03-30
**Status:** Active (M4)

This guide helps you diagnose and resolve common hermetic sandbox failures in Horcrux.

---

## Common Failure: Undeclared Environment Variable

### Symptom

```
[sandbox] PolicyViolation: undeclared_env_var – MY_TOOL_PATH is not in the allowlist
```

### Cause

Your build action reads `MY_TOOL_PATH` from the environment, but it is not in the sandbox allowlist.

### Resolution

**Option A** — Add the variable to the allowlist in your build rule metadata (future: `horcrux.yaml`):

```yaml
env:
  - MY_TOOL_PATH
```

**Option B** — Temporarily switch to `--sandbox=off` to confirm the build succeeds, then identify and declare all required variables:

```bash
horcrux build //my:target --sandbox=off
```

---

## Common Failure: Undeclared Host Path

### Symptom

```
[sandbox] PolicyViolation: undeclared_path – /opt/tools/custom is not in the allowed paths
```

### Cause

Your build action reads a file or directory that is not in the path allowlist.

### Resolution

Declare the path in your build rule metadata (future: `horcrux.yaml`):

```yaml
allowed_paths:
  - host_path: /opt/tools/custom
    writable: false
```

Or pass `--sandbox=off` for migration.

---

## Common Failure: Network Access Denied

### Symptom

```
[sandbox] PolicyViolation: network_access – outbound TCP connection denied
```

### Cause

A build action attempted to access the network (e.g., fetching a dependency at build time), which is blocked by the default `NetworkPolicy::Deny`.

### Resolution

- **Preferred**: Fetch all dependencies before build time and declare them as explicit inputs.
- **For approved cases**: Add the host to `allowed_network_rules` in `horcrux.yaml` (future).
- **Temporary**: Use `--sandbox=off` or `--sandbox=balanced` with a reduced network policy.

---

## Common Failure: Repro-Check FAILED

### Symptom

```
Reproducibility check: FAIL
  Artifacts differed: 2

Differing artifacts:
  - app.debug
  - libcore.a
```

### Common Causes and Fixes

| Cause | Fix |
|-------|-----|
| Embedded build timestamp | Set `SOURCE_DATE_EPOCH` and add it to the allowlist |
| PID or random UUID in output filename | Use deterministic output naming |
| Non-deterministic map iteration | Use sorted containers or `std::map` instead of `std::unordered_map` |
| Debug info with absolute host paths | Use `-ffile-prefix-map=<src>=<dst>` or `-fdebug-prefix-map` |
| Parallel actions writing to the same file | Ensure each action has a dedicated output path |
| Compiler version embedded in binary | Pin compiler version and add to toolchain metadata |

### Debugging Steps

1. Run with `--sandbox=strict` to reveal undeclared dependencies:
   ```bash
   horcrux build //my:target --sandbox=strict --repro-check
   ```

2. Inspect the differing artifacts:
   ```bash
   diff <(xxd build1/app.debug) <(xxd build2/app.debug) | head -40
   ```

3. Use `strings` to look for timestamps or hostnames:
   ```bash
   strings app.debug | grep -E '[0-9]{4}-[0-9]{2}-[0-9]{2}|hostname'
   ```

4. Check for non-deterministic linker or archiver flags:
   ```bash
   ar D libcore.a   # 'D' flag creates deterministic archives (no timestamps)
   ```

---

## Common Failure: Cache Miss After Policy Change

### Symptom

Every build triggers a full rebuild after changing `--sandbox` mode.

### Cause

Horcrux deliberately invalidates cache entries when the sandbox policy changes. The cache key includes a policy fingerprint, so a different policy means a different key.

### Resolution

This is expected and correct behavior. After switching policies, the initial build will be a cache miss but subsequent builds will reuse the cache.

---

## Increasing Diagnostics

Run with `--verbose` to see detailed sandbox policy information:

```bash
horcrux build //my:target --verbose --sandbox=strict
```

The verbose output will show:
- The active sandbox mode and network policy.
- Which env vars were passed through vs. stripped.
- Any policy violations detected during execution.

---

## Escape Hatch: Disabling the Sandbox

For migration or debugging only, you can disable all sandboxing:

```bash
horcrux build //my:target --sandbox=off
```

**Warning**: This disables all hermeticity guarantees and should not be used in CI or production builds.

---

## See Also

- [Hermetic Build Guide](hermetic-build.md)
- [Architecture](architecture.md)
- [Project Status](project-status.md)
