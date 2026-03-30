# Registry and Plugin Usage Guide

> **Milestone:** M5 — Plugin and Registry Ecosystem

This guide explains how to use the Horcrux plugin and registry CLI commands to discover, install, update, and remove plugins.

---

## Overview

Horcrux plugins extend the build system with new rules, adapters, and toolchains. The registry is the distribution mechanism: packages are indexed in one or more registries, resolved to specific versions, installed to the local plugins directory, and pinned in a lockfile for reproducible builds.

---

## Quick Start

```sh
# Search for a plugin
horcrux plugin search wasm

# Install a plugin
horcrux plugin install horcrux-wasm

# List installed plugins
horcrux plugin list

# Update a plugin
horcrux plugin update horcrux-wasm

# Remove a plugin
horcrux plugin remove horcrux-wasm
```

---

## Plugin Commands

### `plugin search <query>`

Search the configured registries for plugins matching the query (case-insensitive, substring match on name and description).

```sh
horcrux plugin search wasm
horcrux plugin search ""          # list all available plugins
```

### `plugin install <name> [version]`

Install a plugin from the registry. If `version` is omitted, the latest available version is installed. A trust prompt is shown before installation.

```sh
horcrux plugin install horcrux-wasm
horcrux plugin install horcrux-wasm 1.2.0
```

### `plugin list`

List all currently installed plugins with their versions and checksums.

```sh
horcrux plugin list
```

### `plugin info <name>`

Show detailed metadata for a plugin available in the registry.

```sh
horcrux plugin info horcrux-wasm
```

### `plugin update <name> [version]`

Update an installed plugin to the latest (or specified) version. The existing installation is removed and the new version is installed.

```sh
horcrux plugin update horcrux-wasm
horcrux plugin update horcrux-wasm 2.0.0
```

### `plugin remove <name>`

Remove an installed plugin and update the lockfile.

```sh
horcrux plugin remove horcrux-wasm
```

### Global Options

All plugin commands accept:

| Option                | Description                                           |
|-----------------------|-------------------------------------------------------|
| `--plugins-dir=DIR`   | Override plugin install directory (default: `~/.horcrux/plugins`) |
| `--lockfile=FILE`     | Override lockfile path (default: `~/.horcrux/plugins.lock`) |
| `--verbose`, `-v`     | Enable verbose logging                                |

---

## Registry Commands

### `registry add <name> <url> [--trusted]`

Add a registry to the local configuration.

```sh
horcrux registry add official https://registry.horcrux.dev
horcrux registry add internal https://registry.mycompany.com --trusted
```

### `registry remove <name>`

Remove a registry from the local configuration.

```sh
horcrux registry remove internal
```

### `registry list`

List all configured registries.

```sh
horcrux registry list
```

### Global Options

| Option            | Description                                                    |
|-------------------|----------------------------------------------------------------|
| `--config=FILE`   | Override registry config file (default: `~/.horcrux/registries.conf`) |
| `--verbose`, `-v` | Enable verbose logging                                         |

---

## Plugin Lockfile

The lockfile (`~/.horcrux/plugins.lock` by default) records the exact set of installed plugins:

```
# Horcrux Plugin Lockfile - do not edit manually
name=horcrux-wasm version=1.2.0 checksum=abc123... registry=https://registry.horcrux.dev
```

### Reproducible Installs

Commit the lockfile to version control so that all team members use the same plugin versions. To restore plugins from the lockfile, use `plugin install` for each locked entry.

---

## Plugin Install Directory Layout

Installed plugins are stored under the plugins directory:

```
~/.horcrux/plugins/
├── horcrux-wasm/
│   └── manifest.toml
└── horcrux-rust/
    └── manifest.toml
```

Each plugin subdirectory contains the plugin's `manifest.toml` and any plugin-specific files.

---

## Offline Mode

When no network is available, `plugin search` and `plugin install` operate against the local in-memory index. If a package is not in the index, the operation fails with `Registry network unavailable`.

To support fully offline teams, mirror the registry to a local HTTP server and add it with `registry add`:

```sh
horcrux registry add offline-mirror https://registry.internal.company.com
```

---

## Trust and Security

1. **First-install trust prompt**: Before installing a new plugin, Horcrux prints a trust prompt listing the plugin's name, version, and origin.
2. **Checksum verification**: If the plugin manifest declares a `checksum`, the downloaded archive is verified before installation.
3. **Permission enforcement**: Plugins must declare all permissions in their manifest. Undeclared permissions are denied.
4. **Compatibility gate**: Plugins that are not compatible with the current Horcrux version are rejected.

See [Plugin API Reference](plugin-api.md#security) for the full security model.
