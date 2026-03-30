// Horcrux - Plugin Loader Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "plugin_loader.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace horcrux::core {

// ─────────────────────────────────────────────────────────────────────────────
// to_string helpers
// ─────────────────────────────────────────────────────────────────────────────

auto to_string(PluginState state) -> std::string {
  switch (state) {
  case PluginState::Unloaded:
    return "unloaded";
  case PluginState::Initialized:
    return "initialized";
  case PluginState::Failed:
    return "failed";
  case PluginState::ShutDown:
    return "shutdown";
  }
  return "unknown";
}

auto to_string(PluginLoaderError error) -> std::string {
  switch (error) {
  case PluginLoaderError::ManifestNotFound:
    return "Plugin manifest not found";
  case PluginLoaderError::ManifestInvalid:
    return "Plugin manifest is invalid";
  case PluginLoaderError::CompatibilityFailed:
    return "Plugin is not compatible with this version of Horcrux";
  case PluginLoaderError::ChecksumFailed:
    return "Plugin checksum verification failed";
  case PluginLoaderError::PermissionViolation:
    return "Plugin requests permissions not allowed by trust policy";
  case PluginLoaderError::AlreadyLoaded:
    return "A plugin with this name is already loaded";
  case PluginLoaderError::InitFailed:
    return "Plugin initialization failed";
  }
  return "Unknown plugin loader error";
}

// ─────────────────────────────────────────────────────────────────────────────
// PluginLoader
// ─────────────────────────────────────────────────────────────────────────────

PluginLoader::PluginLoader(PluginVersion horcrux_version, PluginTrustPolicy trust_policy)
    : horcrux_version_(std::move(horcrux_version)), trust_policy_(std::move(trust_policy)) {
}

auto PluginLoader::validate_candidate(const PluginManifest& manifest,
                                      const std::filesystem::path& install_path)
    -> tl::expected<void, PluginLoaderError> {
  // Compatibility gate
  auto compat_error = validate_plugin_manifest(manifest, horcrux_version_);
  if (compat_error.has_value()) {
    return tl::unexpected(PluginLoaderError::CompatibilityFailed);
  }

  // Permission gate
  auto perm_error = enforce_plugin_permissions(manifest, trust_policy_);
  if (perm_error.has_value()) {
    return tl::unexpected(PluginLoaderError::PermissionViolation);
  }

  // Checksum gate (if manifest specifies a checksum)
  if (manifest.checksum.has_value()) {
    // Look for a plugin binary in the install path
    auto binary_path = install_path / manifest.name;
    auto checksum_result = verify_plugin_checksum(binary_path, manifest);
    if (!checksum_result) {
      return tl::unexpected(PluginLoaderError::ChecksumFailed);
    }
  }

  return {};
}

auto PluginLoader::register_plugin(PluginManifest manifest, std::filesystem::path install_path)
    -> tl::expected<size_t, PluginLoaderError> {
  // Duplicate check
  for (const auto& existing : plugins_) {
    if (existing.manifest.name == manifest.name) {
      return tl::unexpected(PluginLoaderError::AlreadyLoaded);
    }
  }

  auto validation = validate_candidate(manifest, install_path);
  if (!validation) {
    return tl::unexpected(validation.error());
  }

  PluginRecord record{
      .manifest = std::move(manifest),
      .state = PluginState::Unloaded,
      .install_path = std::move(install_path),
  };

  const size_t index = plugins_.size();
  plugins_.push_back(std::move(record));
  return index;
}

auto PluginLoader::scan_directory(const std::filesystem::path& plugins_dir) -> size_t {
  if (!std::filesystem::exists(plugins_dir) || !std::filesystem::is_directory(plugins_dir)) {
    return 0;
  }

  // Collect candidate plugin directories in sorted order for determinism
  std::vector<std::filesystem::path> candidates;
  for (const auto& entry : std::filesystem::directory_iterator(plugins_dir)) {
    if (entry.is_directory()) {
      auto manifest_path = entry.path() / "manifest.toml";
      if (std::filesystem::exists(manifest_path)) {
        candidates.push_back(entry.path());
      }
    }
  }
  std::sort(candidates.begin(), candidates.end());

  size_t loaded = 0;
  for (const auto& dir : candidates) {
    auto manifest_path = dir / "manifest.toml";
    std::ifstream file(manifest_path);
    if (!file) {
      continue;
    }

    std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    auto manifest_result = parse_plugin_manifest(content);
    if (!manifest_result) {
      continue; // Skip invalid manifests
    }

    auto reg_result = register_plugin(std::move(*manifest_result), dir);
    if (reg_result) {
      ++loaded;
    }
  }

  return loaded;
}

void PluginLoader::initialize_all() {
  // Sort plugins alphabetically by name for deterministic init order
  std::sort(plugins_.begin(), plugins_.end(), [](const PluginRecord& a, const PluginRecord& b) {
    return a.manifest.name < b.manifest.name;
  });

  for (auto& plugin : plugins_) {
    if (plugin.state != PluginState::Unloaded) {
      continue;
    }
    // In-process model: no dynamic library loading in M5.
    // Lifecycle hook = mark as initialized.
    plugin.state = PluginState::Initialized;
  }
}

void PluginLoader::shutdown_all() {
  // Shutdown in reverse initialization order
  for (auto it = plugins_.rbegin(); it != plugins_.rend(); ++it) {
    if (it->state == PluginState::Initialized) {
      it->state = PluginState::ShutDown;
    }
  }
}

auto PluginLoader::plugins() const -> const std::vector<PluginRecord>& {
  return plugins_;
}

auto PluginLoader::find_plugin(std::string_view name) const -> const PluginRecord* {
  for (const auto& p : plugins_) {
    if (p.manifest.name == name) {
      return &p;
    }
  }
  return nullptr;
}

auto PluginLoader::count_in_state(PluginState state) const -> size_t {
  return static_cast<size_t>(
      std::count_if(plugins_.begin(), plugins_.end(),
                    [state](const PluginRecord& p) { return p.state == state; }));
}

} // namespace horcrux::core
