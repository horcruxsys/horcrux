# Plugin API Reference

> **Milestone:** M5 — Plugin and Registry Ecosystem

This document describes the stable plugin API for Horcrux M5, including the manifest schema, lifecycle hooks, versioning policy, and extension point contracts.

---

## Overview

Horcrux plugins are extensions that add new rule adapters, toolchains, or integrations without modifying the core build system. Each plugin:

1. Declares its identity and capabilities in a **manifest**.
2. Exposes one or more **extension points** (rules, adapters, toolchains).
3. Declares the **permissions** it requires.
4. Is compatible with a specified **Horcrux version range**.

---

## Plugin Manifest Schema

Every plugin must include a `manifest.toml` file in its root directory.

### Required Fields

| Field                  | Type   | Description                                          |
|------------------------|--------|------------------------------------------------------|
| `name`                 | string | Unique plugin identifier (e.g., `horcrux-wasm`)      |
| `version`              | string | SemVer version string (e.g., `"1.2.3"`)              |
| `author`               | string | Author name or contact                               |
| `license`              | string | SPDX license identifier (e.g., `"MIT"`, `"Apache-2.0"`) |

### Optional Fields

| Field                      | Type   | Description                                              |
|----------------------------|--------|----------------------------------------------------------|
| `description`              | string | Short description of the plugin                          |
| `min_horcrux_version`      | string | Minimum compatible Horcrux version (default: `"0.0.0"`) |
| `max_horcrux_version`      | string | Maximum compatible Horcrux version (omit for no upper bound) |
| `checksum`                 | string | SHA-256 hex digest of the plugin binary for verification |
| `permissions.filesystem_read`  | bool | Requests read access to filesystem paths               |
| `permissions.filesystem_write` | bool | Requests write access to output directories            |
| `permissions.network_access`   | bool | Requests outbound network access                       |
| `permissions.process_spawn`    | bool | Requests ability to spawn subprocesses                 |

### Extension Blocks

Declare each extension point in a separate `[extension]` block:

| Field  | Type   | Values                          | Description                     |
|--------|--------|---------------------------------|---------------------------------|
| `kind` | string | `"rule"`, `"adapter"`, `"toolchain"` | Type of extension point    |
| `name` | string | any                             | Name exposed by this extension  |

### Example Manifest

```toml
name = "horcrux-wasm"
version = "1.2.0"
author = "Alice"
license = "MIT"
description = "WebAssembly rule support for Horcrux"
min_horcrux_version = "0.1.0"
max_horcrux_version = "2.0.0"
checksum = "abc123..."

permissions.filesystem_read = true
permissions.filesystem_write = true
permissions.network_access = false
permissions.process_spawn = false

[extension]
kind = "rule"
name = "wasm_binary"

[extension]
kind = "toolchain"
name = "wasm-tools"
```

---

## Plugin Lifecycle

Plugins go through the following lifecycle states:

```
Unloaded → Initialized → ShutDown
              ↓
           Failed
```

| State         | Description                                              |
|---------------|----------------------------------------------------------|
| `Unloaded`    | Manifest parsed and registered, not yet initialized      |
| `Initialized` | Init hook completed; extension points are active         |
| `Failed`      | Init hook returned an error; plugin is inactive          |
| `ShutDown`    | Shutdown hook completed; plugin is inactive              |

### Lifecycle Hooks

Horcrux calls lifecycle hooks in deterministic **alphabetical order** by plugin name during initialization, and **reverse alphabetical order** during shutdown. This ensures reproducible behavior across runs.

1. **Discovery** — Plugin directory is scanned; manifest is parsed and validated.
2. **Load/Initialize** — Compatibility and permission gates are checked; plugin is activated.
3. **Rule Registration** — Plugin extension points are registered with the core build system.
4. **Shutdown** — Called during Horcrux exit; cleanup is performed.

---

## Compatibility and Versioning

### Version Format

Plugin versions use [SemVer](https://semver.org/): `MAJOR.MINOR.PATCH`.

### Compatibility Gate

Before loading a plugin, Horcrux checks:

1. `min_horcrux_version ≤ current_version`
2. If `max_horcrux_version` is non-zero: `current_version ≤ max_horcrux_version`

If either check fails, the plugin is not loaded and an error is reported.

### Breaking Change Policy

- **MAJOR** version bumps signal API-breaking changes.
- Plugins declaring `max_horcrux_version` must be updated when a new Horcrux major version is released.
- Horcrux guarantees backward compatibility within a major version.

---

## Permission Model

Plugins must explicitly declare every permission they need. Undeclared permissions are denied at runtime.

| Permission           | Default | Description                                   |
|----------------------|---------|-----------------------------------------------|
| `filesystem_read`    | false   | May read arbitrary paths                      |
| `filesystem_write`   | false   | May write to designated output directories    |
| `network_access`     | false   | May make outbound network connections         |
| `process_spawn`      | false   | May spawn child processes                     |

### Trust Policies

Horcrux applies a `PluginTrustPolicy` to gate permission requests:

| Policy    | Description                                               |
|-----------|-----------------------------------------------------------|
| `default` | Allows fs_read, fs_write, process_spawn; denies network   |
| `strict`  | Allows fs_read, fs_write; denies network and process_spawn|

---

## ABI/API Versioning

The plugin API version is tied to the Horcrux core version. The manifest `min_horcrux_version` / `max_horcrux_version` fields are the primary compatibility mechanism.

In M5, plugins run **in-process**. A future milestone will introduce shared-library ABI versioning with explicit symbol exports.

---

## Extension Point Reference

### `rule`

Registers a new build rule kind (e.g., `wasm_binary`). The rule name becomes available in `horcrux.yaml` target definitions.

### `adapter`

Registers a new language adapter (e.g., `zig`). The adapter handles `parse_target`, `plan_actions`, and `compute_cache_key` for the registered target kinds.

### `toolchain`

Registers a new toolchain descriptor (e.g., `ndk-r26`). Toolchain descriptors define compiler paths, flags, and sysroots.

---

## Security

- Plugin packages are verified via SHA-256 checksum at install time.
- Plugins that fail verification are not installed.
- Plugin faults are isolated: one plugin failure does not crash the core runtime.
- Trust prompts are shown for first-time plugin installs.

See [Security and Trust Model](plugin-authoring.md#security-and-trust) for more details.
