# Plugin Authoring Tutorial

> **Milestone:** M5 — Plugin and Registry Ecosystem

This tutorial walks through creating, testing, and publishing a Horcrux plugin. By the end you will have a working plugin skeleton that registers a new rule type.

---

## Prerequisites

- Horcrux ≥ 0.1.0 installed.
- A C++23 compiler (GCC 14+ or Clang 18+).
- CMake ≥ 3.25.

---

## Plugin Repository Layout

A Horcrux plugin is a directory (or repository) with the following structure:

```
horcrux-myplugin/
├── manifest.toml          # Plugin manifest (required)
├── CMakeLists.txt         # Build system (if plugin ships a shared library)
├── src/
│   └── myplugin.cpp       # Plugin implementation
├── tests/
│   └── myplugin_test.cpp  # Plugin tests
└── README.md
```

---

## Step 1: Write the Manifest

Create `manifest.toml` in the plugin root:

```toml
name = "horcrux-myplugin"
version = "0.1.0"
author = "Your Name <you@example.com>"
license = "MIT"
description = "Adds support for MyLanguage in Horcrux"
min_horcrux_version = "0.1.0"

permissions.filesystem_read = true
permissions.filesystem_write = true
permissions.process_spawn = true

[extension]
kind = "rule"
name = "my_binary"

[extension]
kind = "adapter"
name = "myplugin"
```

### Manifest Checklist

- [ ] `name` is unique and follows `horcrux-<lang>` or `horcrux-<feature>` convention.
- [ ] `version` uses SemVer.
- [ ] `license` is a valid [SPDX identifier](https://spdx.org/licenses/).
- [ ] `min_horcrux_version` reflects the oldest core API your plugin uses.
- [ ] Only permissions your plugin actually needs are declared.
- [ ] Each extension block has a non-empty `kind` (`rule`, `adapter`, or `toolchain`) and `name`.

---

## Step 2: Implement the Plugin

Plugins are in-process in Horcrux M5. They integrate by linking against `horcrux_core` and implementing the `Adapter` interface.

```cpp
// src/myplugin.cpp
#include <horcrux/core/adapter.h>

namespace myplugin {

class MyPluginAdapter : public horcrux::core::Adapter {
public:
  [[nodiscard]] auto info() const -> const horcrux::core::AdapterInfo& override {
    static const horcrux::core::AdapterInfo kInfo{
        .name = "myplugin",
        .version = "0.1.0",
        .supported_kinds = {"my_binary"},
    };
    return kInfo;
  }

  [[nodiscard]] auto parse_target(const horcrux::core::BuildNode& node)
      -> tl::expected<horcrux::core::AdapterTarget, horcrux::core::AdapterError> override {
    horcrux::core::AdapterTarget t;
    t.label = node.label();
    t.kind = node.kind();
    return t;
  }

  [[nodiscard]] auto plan_actions(const horcrux::core::AdapterTarget& target,
                                   const horcrux::core::BuildGraph&,
                                   const std::string& output_root)
      -> tl::expected<std::vector<horcrux::core::BuildAction>,
                      horcrux::core::AdapterError> override {
    horcrux::core::BuildAction action;
    action.kind = horcrux::core::BuildAction::Kind::Custom;
    action.description = "Build " + target.label;
    action.command = {"my-compiler", "--output", output_root + "/out"};
    return std::vector{std::move(action)};
  }

  [[nodiscard]] auto compute_cache_key(const horcrux::core::AdapterTarget& target)
      -> tl::expected<horcrux::core::Hash, horcrux::core::AdapterError> override {
    std::string key_input = target.label + target.kind;
    std::vector<uint8_t> bytes(key_input.begin(), key_input.end());
    return horcrux::core::compute_sha256(bytes);
  }

  [[nodiscard]] auto diagnostics() const
      -> const std::vector<horcrux::core::Diagnostic>& override {
    return diagnostics_;
  }

private:
  std::vector<horcrux::core::Diagnostic> diagnostics_;
};

} // namespace myplugin
```

---

## Step 3: Write Tests

```cpp
// tests/myplugin_test.cpp
#include <gtest/gtest.h>
#include "../src/myplugin.cpp"

TEST(MyPluginAdapterTest, SupportsMyBinaryKind) {
  myplugin::MyPluginAdapter adapter;
  EXPECT_TRUE(adapter.supports_kind("my_binary"));
  EXPECT_FALSE(adapter.supports_kind("cc_binary"));
}
```

Run tests with:

```sh
cmake -B build && cmake --build build && ctest --output-on-failure
```

---

## Step 4: Validate the Manifest

Use the `PluginManifest` parser to validate your manifest before publishing:

```cpp
#include <horcrux/core/plugin_manifest.h>

auto result = horcrux::core::parse_plugin_manifest(manifest_text);
if (!result) {
    std::cerr << "Manifest error: " << horcrux::core::to_string(result.error()) << "\n";
}
auto compat_error = horcrux::core::validate_plugin_manifest(*result, current_version);
if (compat_error) {
    std::cerr << "Compatibility error: " << *compat_error << "\n";
}
```

---

## Step 5: Compute and Embed a Checksum

After building your plugin, compute the SHA-256 of the binary and embed it in the manifest:

```sh
sha256sum my-plugin-binary | awk '{print $1}'
# e.g.: a3f2...
```

Then set `checksum = "a3f2..."` in `manifest.toml`. The Horcrux registry client will verify this on install.

---

## Step 6: Publish

### Registry Package Format

A publishable plugin package consists of:

1. The `manifest.toml` file.
2. Any compiled binaries or resources.
3. A `checksum` field in the manifest matching the SHA-256 of the binary.

### Submit to the Official Registry

1. Fork the Horcrux registry repository (coming in a future milestone).
2. Add your package metadata to the index.
3. Open a pull request.
4. Automated CI runs compatibility tests against supported Horcrux versions.
5. Once merged, your plugin appears in `horcrux plugin search`.

---

## Security and Trust

### What plugin authors must do

- Declare **only** the permissions the plugin needs. Requesting unnecessary permissions will trigger warnings and may prevent installation under strict trust policies.
- Always embed a `checksum` in the manifest before publishing to a public registry.
- Use `min_horcrux_version` / `max_horcrux_version` accurately to prevent incompatible loads.

### What Horcrux does

- Shows a trust prompt to users before installing a new plugin.
- Verifies the `checksum` against the downloaded archive.
- Enforces the active `PluginTrustPolicy` before loading.
- Isolates plugin faults: a plugin that panics or throws does not crash the core runtime.

---

## Compatibility Testing

Run your plugin against multiple Horcrux versions using the compatibility test harness:

```sh
horcrux plugin install horcrux-myplugin --plugins-dir=./test-plugins
```

Check the plugin loads successfully:

```sh
# Scan and initialize in a PluginLoader
PluginLoader loader(PluginVersion{0, 1, 0});
loader.scan_directory("./test-plugins");
loader.initialize_all();
assert(loader.count_in_state(PluginState::Initialized) == 1);
```

---

## SDK Starter Template

A starter template repository is provided at `examples/plugin-sdk/` in the Horcrux repository. It includes:

- A minimal `manifest.toml`
- A skeleton `Adapter` implementation
- CMake build configuration
- A basic test suite
- CI workflow for plugin compatibility testing
