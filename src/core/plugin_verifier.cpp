// Horcrux - Plugin Verifier Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "plugin_verifier.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace horcrux::core {

// ─────────────────────────────────────────────────────────────────────────────
// VerifierError to_string
// ─────────────────────────────────────────────────────────────────────────────

auto to_string(VerifierError error) -> std::string {
  switch (error) {
  case VerifierError::ChecksumMismatch:
    return "Plugin checksum does not match manifest";
  case VerifierError::MissingFile:
    return "Plugin file not found";
  case VerifierError::PermissionDenied:
    return "Plugin requests disallowed permissions";
  case VerifierError::HashError:
    return "Failed to compute plugin file hash";
  }
  return "Unknown verifier error";
}

// ─────────────────────────────────────────────────────────────────────────────
// compute_file_sha256
// ─────────────────────────────────────────────────────────────────────────────

auto compute_file_sha256(const std::filesystem::path& path)
    -> tl::expected<Hash, VerifierError> {
  if (!std::filesystem::exists(path)) {
    return tl::unexpected(VerifierError::MissingFile);
  }

  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return tl::unexpected(VerifierError::MissingFile);
  }

  std::vector<uint8_t> content{std::istreambuf_iterator<char>(file),
                                std::istreambuf_iterator<char>()};
  return compute_sha256(std::span<const uint8_t>(content));
}

// ─────────────────────────────────────────────────────────────────────────────
// verify_plugin_checksum
// ─────────────────────────────────────────────────────────────────────────────

auto verify_plugin_checksum(const std::filesystem::path& plugin_path,
                             const PluginManifest& manifest)
    -> tl::expected<void, VerifierError> {
  // If no checksum declared, skip verification
  if (!manifest.checksum.has_value()) {
    return {};
  }

  auto hash_result = compute_file_sha256(plugin_path);
  if (!hash_result) {
    return tl::unexpected(hash_result.error());
  }

  const std::string computed = hash_to_string(*hash_result);
  if (computed != *manifest.checksum) {
    return tl::unexpected(VerifierError::ChecksumMismatch);
  }

  return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// enforce_plugin_permissions
// ─────────────────────────────────────────────────────────────────────────────

auto enforce_plugin_permissions(const PluginManifest& manifest,
                                 const PluginTrustPolicy& policy)
    -> std::optional<std::string> {
  if (manifest.permissions.filesystem_read && !policy.allow_filesystem_read) {
    return "Plugin '" + manifest.name +
           "' requests filesystem_read which is not allowed by trust policy";
  }
  if (manifest.permissions.filesystem_write && !policy.allow_filesystem_write) {
    return "Plugin '" + manifest.name +
           "' requests filesystem_write which is not allowed by trust policy";
  }
  if (manifest.permissions.network_access && !policy.allow_network_access) {
    return "Plugin '" + manifest.name +
           "' requests network_access which is not allowed by trust policy";
  }
  if (manifest.permissions.process_spawn && !policy.allow_process_spawn) {
    return "Plugin '" + manifest.name +
           "' requests process_spawn which is not allowed by trust policy";
  }
  return std::nullopt;
}

} // namespace horcrux::core
