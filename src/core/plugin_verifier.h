// Horcrux - Plugin Verifier (Checksum & Permission Enforcement)
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <tl/expected.hpp>

#include "local_cache.h"
#include "plugin_manifest.h"

namespace horcrux::core {

/// @brief Error types for plugin verification
enum class VerifierError {
  ChecksumMismatch, ///< Computed checksum does not match manifest
  MissingFile,      ///< Plugin binary/archive not found
  PermissionDenied, ///< Plugin requests permissions not allowed by policy
  HashError,        ///< Failed to compute or parse hash
};

/// @brief Convert VerifierError to human-readable string
[[nodiscard]] auto to_string(VerifierError error) -> std::string;

/// @brief Global permission policy controlling what plugins may request
struct PluginTrustPolicy {
  bool allow_filesystem_read = true;  ///< Permit plugins to declare filesystem_read
  bool allow_filesystem_write = true; ///< Permit plugins to declare filesystem_write
  bool allow_network_access = false;  ///< Permit plugins to declare network_access
  bool allow_process_spawn = true;    ///< Permit plugins to declare process_spawn

  /// @brief Default permissive policy (all except network)
  [[nodiscard]] static auto default_policy() -> PluginTrustPolicy {
    return {};
  }

  /// @brief Strict policy: no network, no process spawn
  [[nodiscard]] static auto strict_policy() -> PluginTrustPolicy {
    return {.allow_filesystem_read = true,
            .allow_filesystem_write = true,
            .allow_network_access = false,
            .allow_process_spawn = false};
  }
};

/// @brief Verify the SHA-256 checksum of a plugin file against the manifest
///
/// Reads the file at @p plugin_path, computes its SHA-256, and compares it
/// against @p manifest.checksum (hex string).  If the manifest has no
/// checksum, the check is skipped and success is returned.
///
/// @param plugin_path Path to the plugin binary or archive
/// @param manifest    Parsed plugin manifest
/// @return std::nullopt on success, VerifierError on failure
[[nodiscard]] auto
verify_plugin_checksum(const std::filesystem::path& plugin_path,
                       const PluginManifest& manifest) -> tl::expected<void, VerifierError>;

/// @brief Enforce that a plugin's declared permissions are allowed by policy
///
/// @param manifest Plugin manifest containing requested permissions
/// @param policy   Trust policy enforced by the runtime
/// @return std::nullopt on success, description of violation on failure
[[nodiscard]] auto
enforce_plugin_permissions(const PluginManifest& manifest,
                           const PluginTrustPolicy& policy) -> std::optional<std::string>;

/// @brief Compute SHA-256 of a file on disk
///
/// @param path File to hash
/// @return Hash on success, VerifierError on failure
[[nodiscard]] auto
compute_file_sha256(const std::filesystem::path& path) -> tl::expected<Hash, VerifierError>;

} // namespace horcrux::core
