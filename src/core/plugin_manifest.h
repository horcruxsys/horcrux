// Horcrux - Plugin Manifest Schema and Parser
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

namespace horcrux::core {

/// @brief Semantic version for plugin and compatibility range
struct PluginVersion {
  uint32_t ver_major = 0;
  uint32_t ver_minor = 0;
  uint32_t ver_patch = 0;

  [[nodiscard]] auto to_string() const -> std::string;

  auto operator==(const PluginVersion&) const -> bool = default;
  auto operator<=>(const PluginVersion&) const = default;

  /// @brief Parse "MAJOR.MINOR.PATCH" string; returns error if malformed
  [[nodiscard]] static auto parse(std::string_view s) -> tl::expected<PluginVersion, std::string>;
};

/// @brief Declared permissions a plugin requires
struct PluginPermissions {
  bool filesystem_read = false;  ///< May read arbitrary filesystem paths
  bool filesystem_write = false; ///< May write to output directories
  bool network_access = false;   ///< May perform outbound network calls
  bool process_spawn = false;    ///< May spawn subprocesses
};

/// @brief Exposed extension point declared in the manifest
struct PluginExtension {
  std::string kind; ///< "rule", "adapter", "toolchain"
  std::string name; ///< e.g., "wasm_binary", "zig", "ndk-r26"
};

/// @brief Parsed and validated plugin manifest
///
/// Corresponds to the plugin's manifest.toml / manifest.yaml file.
struct PluginManifest {
  // Identity
  std::string name;      ///< Unique plugin name (e.g., "horcrux-wasm")
  PluginVersion version; ///< Plugin version
  std::string author;    ///< Author name or contact
  std::string license;   ///< SPDX license identifier (e.g., "MIT")
  std::string description;

  // Compatibility
  PluginVersion min_horcrux_version; ///< Minimum required Horcrux core version
  PluginVersion max_horcrux_version; ///< Maximum compatible Horcrux core version (inclusive)

  // Extensions exposed by this plugin
  std::vector<PluginExtension> extensions;

  // Required permissions
  PluginPermissions permissions;

  // Optional checksum of the plugin binary (SHA-256 hex)
  std::optional<std::string> checksum;
};

/// @brief Error types for manifest operations
enum class ManifestError {
  ParseError,       ///< Manifest file could not be parsed
  MissingField,     ///< A required field is absent
  InvalidVersion,   ///< Version string is malformed
  InvalidLicense,   ///< License identifier is not recognized
  InvalidExtension, ///< Extension entry is malformed
};

/// @brief Convert ManifestError to human-readable string
[[nodiscard]] auto to_string(ManifestError error) -> std::string;

/// @brief Parse and validate a plugin manifest from a TOML-like key=value text
///
/// The parser accepts a simplified line-oriented format:
///   name = "horcrux-wasm"
///   version = "1.2.3"
///   author = "Alice"
///   license = "MIT"
///   description = "WebAssembly support"
///   min_horcrux_version = "0.1.0"
///   max_horcrux_version = "1.0.0"
///   permissions.filesystem_read = true
///   permissions.network_access = false
///   [extension]
///   kind = "rule"
///   name = "wasm_binary"
///
/// @param text Raw manifest content
/// @return Parsed PluginManifest or error
[[nodiscard]] auto
parse_plugin_manifest(std::string_view text) -> tl::expected<PluginManifest, ManifestError>;

/// @brief Validate a parsed manifest against the running Horcrux version
///
/// @param manifest Parsed manifest to validate
/// @param horcrux_version Current Horcrux core version
/// @return std::nullopt on success, error string on failure
[[nodiscard]] auto
validate_plugin_manifest(const PluginManifest& manifest,
                         const PluginVersion& horcrux_version) -> std::optional<std::string>;

} // namespace horcrux::core
