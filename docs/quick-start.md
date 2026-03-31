# Quick Start Guide

This guide gets you from zero to a working Horcrux build in under five minutes.

## Prerequisites

| Requirement | Minimum version | Notes |
|-------------|-----------------|-------|
| C++23 compiler | GCC 13+ or Clang 17+ | MSVC 2022+ on Windows |
| CMake | 3.25+ | Used only to build Horcrux itself |
| Ninja | 1.11+ | Recommended generator |
| OpenSSL | 1.1.1+ | Required by the plugin verifier |
| Git | 2.40+ | Required for source builds |

## Installation

### From a Release Archive (Recommended)

1. Download the latest release from the [releases page](https://github.com/horcruxsys/horcrux/releases).

2. Verify the checksum:

   ```bash
   sha256sum -c horcrux-2026.0401.0-checksums.txt
   ```

3. Extract and install:

   ```bash
   tar -xzf horcrux-2026.0401.0-linux-x86_64.tar.gz
   sudo cp horcrux /usr/local/bin/
   ```

4. Confirm the installation:

   ```bash
   horcrux --version
   # Horcrux 2026.0401.0
   ```

### Building from Source

```bash
git clone https://github.com/horcruxsys/horcrux.git
cd horcrux

cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/local.cmake

cmake --build build -j "$(nproc)"
sudo cmake --install build
```

## Your First Build

### 1. Verify your environment

```bash
horcrux doctor
```

The `doctor` command checks that required tools are available and that your
environment is compatible with hermetic builds.

### 2. Build a target

```bash
# Build all targets in the current workspace
horcrux build //...

# Build a specific target
horcrux build //src:my_binary
```

### 3. Run tests

```bash
# Run all test targets
horcrux test //...

# Run tests for a specific package
horcrux test //src/core:...
```

### 4. Query the build graph

```bash
# List all targets
horcrux query //...

# Show transitive dependencies of a target
horcrux query --transitive //src:my_binary

# Output as JSON
horcrux query --output=json //...
```

### 5. Clean build outputs

```bash
# Remove all build outputs and cache
horcrux clean --all

# Remove only cached artifacts
horcrux clean --cache
```

## Hermetic Builds

Horcrux builds are hermetic by default in balanced mode. For stricter
reproducibility guarantees:

```bash
# Balanced sandboxing (recommended default)
horcrux build --sandbox=balanced //...

# Strict sandboxing with full isolation
horcrux build --sandbox=strict //...

# Reproducibility check (runs the build twice and compares artifacts)
horcrux build --repro-check //...
```

See [hermetic-build.md](hermetic-build.md) for full details.

## Importing a Gradle Project

```bash
# Import a Gradle project from the current directory
horcrux import .

# Import from a specific path
horcrux import /path/to/android/project
```

## Managing Plugins

```bash
# Search for plugins in the default registry
horcrux plugin search kotlin

# Install a plugin
horcrux plugin install horcrux-kotlin-plugin

# List installed plugins
horcrux plugin list

# Update all plugins
horcrux plugin update
```

## Next Steps

- [Migration Guide](migration-guide.md) — upgrading from other build systems
- [Architecture Overview](architecture.md) — how Horcrux works internally
- [CLI Reference](cli-implementation.md) — full command reference
- [Hermetic Builds](hermetic-build.md) — sandboxing and reproducibility
- [Plugin Authoring](plugin-authoring.md) — extending Horcrux
- [Troubleshooting](troubleshooting.md) — common issues and fixes
