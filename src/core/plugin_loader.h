// Horcrux - Plugin Loader and Lifecycle Orchestrator
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "plugin_manifest.h"
#include "plugin_verifier.h"

namespace horcrux::core {

/// @brief Runtime state of an installed plugin
enum class PluginState {
  Unloaded,      ///< Manifest parsed but plugin not yet initialized
  Initialized,   ///< Lifecycle init hook completed successfully
  Failed,        ///< Init or registration hook returned an error
  ShutDown,      ///< Shutdown hook completed; plugin is inactive
};

/// @brief Convert PluginState to human-readable string
[[nodiscard]] auto to_string(PluginState state) -> std::string;

/// @brief In-process representation of a loaded plugin
///
/// Horcrux M5 implements an in-process plugin model: plugins are shared
/// libraries or statically registered modules.  The loader manages their
/// lifecycle hooks in deterministic order.
struct PluginRecord {
  PluginManifest manifest;
  PluginState state = PluginState::Unloaded;
  std::filesystem::path install_path; ///< Where the plugin is stored on disk
  std::optional<std::string> error;   ///< Last error message, if state == Failed
};

/// @brief Error types for plugin loading operations
enum class PluginLoaderError {
  ManifestNotFound,     ///< manifest file missing from install path
  ManifestInvalid,      ///< Manifest failed parse or validation
  CompatibilityFailed,  ///< Plugin not compatible with current Horcrux version
  ChecksumFailed,       ///< Checksum verification failed
  PermissionViolation,  ///< Plugin requests disallowed permissions
  AlreadyLoaded,        ///< Plugin with this name is already registered
  InitFailed,           ///< Plugin lifecycle init hook returned error
};

/// @brief Convert PluginLoaderError to human-readable string
[[nodiscard]] auto to_string(PluginLoaderError error) -> std::string;

/// @brief Orchestrates plugin discovery, validation, and lifecycle management
///
/// The PluginLoader:
///   - Scans a plugin directory for installed plugins
///   - Parses and validates each plugin manifest
///   - Enforces compatibility and permission gates
///   - Maintains deterministic load order (alphabetical by plugin name)
///   - Isolates faults: one plugin failure does not abort other plugins
class PluginLoader {
public:
  /// @brief Construct loader for the given Horcrux version and trust policy
  explicit PluginLoader(PluginVersion horcrux_version,
                        PluginTrustPolicy trust_policy = PluginTrustPolicy::default_policy());

  // Non-copyable
  PluginLoader(const PluginLoader&) = delete;
  PluginLoader& operator=(const PluginLoader&) = delete;
  PluginLoader(PluginLoader&&) = default;
  PluginLoader& operator=(PluginLoader&&) = default;

  /// @brief Register a plugin from a manifest (used for testing / static plugins)
  ///
  /// @param manifest  Plugin manifest
  /// @param install_path Directory where plugin is installed
  /// @return Plugin index on success, error on failure
  [[nodiscard]] auto register_plugin(PluginManifest manifest,
                                     std::filesystem::path install_path)
      -> tl::expected<size_t, PluginLoaderError>;

  /// @brief Scan a directory for plugins and register them all
  ///
  /// Each subdirectory containing a "manifest.toml" file is treated as a plugin.
  /// Failures are recorded per-plugin; the scan continues on error.
  ///
  /// @param plugins_dir Directory to scan (e.g., ~/.horcrux/plugins)
  /// @return Number of successfully registered plugins
  auto scan_directory(const std::filesystem::path& plugins_dir) -> size_t;

  /// @brief Initialize all registered plugins in deterministic (alphabetical) order
  ///
  /// Plugins that fail initialization are marked Failed; others continue.
  void initialize_all();

  /// @brief Shutdown all initialized plugins in reverse order
  void shutdown_all();

  /// @brief Get all registered plugins (ordered by load index)
  [[nodiscard]] auto plugins() const -> const std::vector<PluginRecord>&;

  /// @brief Find a plugin record by name (case-sensitive)
  [[nodiscard]] auto find_plugin(std::string_view name) const
      -> const PluginRecord*;

  /// @brief Get the count of plugins in a given state
  [[nodiscard]] auto count_in_state(PluginState state) const -> size_t;

private:
  PluginVersion horcrux_version_;
  PluginTrustPolicy trust_policy_;
  std::vector<PluginRecord> plugins_;

  /// Internal: validate + checksum-check a candidate plugin
  [[nodiscard]] auto validate_candidate(const PluginManifest& manifest,
                                        const std::filesystem::path& install_path)
      -> tl::expected<void, PluginLoaderError>;
};

} // namespace horcrux::core
