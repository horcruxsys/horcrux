// Horcrux - Registry Client
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "plugin_manifest.h"

namespace horcrux::core {

/// @brief Metadata for a single package available in the registry
struct RegistryPackage {
  std::string name;
  PluginVersion version;
  std::string author;
  std::string description;
  std::string license;
  std::string download_url;
  std::string checksum; ///< Expected SHA-256 hex of the downloaded archive
};

/// @brief A resolved entry in the plugin lockfile
struct LockfileEntry {
  std::string name;
  PluginVersion version;
  std::string checksum;     ///< SHA-256 of the installed archive
  std::string registry_url; ///< Registry this package was resolved from
};

/// @brief Plugin lockfile: records the exact resolved set of plugins
struct PluginLockfile {
  std::vector<LockfileEntry> entries;

  /// @brief Find an entry by plugin name
  [[nodiscard]] auto find(std::string_view name) const -> const LockfileEntry*;
};

/// @brief Registry configuration entry
struct RegistryConfig {
  std::string name;     ///< Human-readable registry name (e.g., "official")
  std::string url;      ///< Base URL of the registry
  bool trusted = false; ///< Whether this registry is implicitly trusted
};

/// @brief Error types for registry operations
enum class RegistryError {
  NetworkUnavailable, ///< Cannot reach registry (offline mode)
  PackageNotFound,    ///< Package not found in registry index
  VersionNotFound,    ///< Requested version not in registry
  DownloadFailed,     ///< Archive download failed
  ChecksumMismatch,   ///< Downloaded archive checksum mismatch
  InstallFailed,      ///< Failed to unpack or install plugin
  AlreadyInstalled,   ///< Plugin is already installed at requested version
  LockfileError,      ///< Failed to read/write lockfile
  InvalidConfig,      ///< Registry config is malformed
};

/// @brief Convert RegistryError to human-readable string
[[nodiscard]] auto to_string(RegistryError error) -> std::string;

/// @brief Simulated in-memory registry client
///
/// The RegistryClient manages a set of configured registries and provides
/// package search, resolution, install, update, and remove workflows.
///
/// In M5, the client uses an in-memory package index (no real HTTP).
/// The interface is designed so that a real HTTP client can be plugged in
/// transparently in a future milestone.
class RegistryClient {
public:
  explicit RegistryClient(std::filesystem::path plugins_dir, std::filesystem::path lockfile_path);

  // Non-copyable
  RegistryClient(const RegistryClient&) = delete;
  RegistryClient& operator=(const RegistryClient&) = delete;
  RegistryClient(RegistryClient&&) = default;
  RegistryClient& operator=(RegistryClient&&) = default;

  // ── Registry management ─────────────────────────────────────────────────

  /// @brief Add a registry configuration
  void add_registry(RegistryConfig config);

  /// @brief Remove a registry by name; returns true if found and removed
  auto remove_registry(std::string_view name) -> bool;

  /// @brief List all configured registries
  [[nodiscard]] auto list_registries() const -> const std::vector<RegistryConfig>&;

  // ── Package index ────────────────────────────────────────────────────────

  /// @brief Seed the in-memory index with a known package (used by tests and
  ///        future HTTP index fetch)
  void seed_package(RegistryPackage package);

  /// @brief Search packages by name substring (case-insensitive)
  [[nodiscard]] auto search(std::string_view query) const -> std::vector<RegistryPackage>;

  /// @brief Get metadata for a specific package
  [[nodiscard]] auto package_info(std::string_view name) const -> std::optional<RegistryPackage>;

  // ── Install / update / remove ────────────────────────────────────────────

  /// @brief Install a package into the plugins directory
  ///
  /// Simulates download + checksum verification + extraction.
  /// Writes a manifest.toml to plugins_dir/name/manifest.toml.
  ///
  /// @param name    Package name
  /// @param version Optional explicit version; latest if empty
  /// @param prompt_trust If true, print a trust prompt before installing
  [[nodiscard]] auto
  install(std::string_view name, std::optional<PluginVersion> version = std::nullopt,
          bool prompt_trust = false) -> tl::expected<LockfileEntry, RegistryError>;

  /// @brief Update an installed package to the latest (or given) version
  [[nodiscard]] auto update(std::string_view name,
                            std::optional<PluginVersion> version = std::nullopt)
      -> tl::expected<LockfileEntry, RegistryError>;

  /// @brief Remove an installed package
  [[nodiscard]] auto remove(std::string_view name) -> tl::expected<void, RegistryError>;

  // ── Lockfile ─────────────────────────────────────────────────────────────

  /// @brief Read the lockfile from disk (returns empty lockfile if not found)
  [[nodiscard]] auto read_lockfile() const -> PluginLockfile;

  /// @brief Write the lockfile to disk
  [[nodiscard]] auto
  write_lockfile(const PluginLockfile& lockfile) const -> tl::expected<void, RegistryError>;

  /// @brief List all installed plugins (from the plugins directory)
  [[nodiscard]] auto list_installed() const -> std::vector<LockfileEntry>;

private:
  std::filesystem::path plugins_dir_;
  std::filesystem::path lockfile_path_;
  std::vector<RegistryConfig> registries_;
  std::vector<RegistryPackage> index_; ///< In-memory package index

  /// Find best matching package (latest version first)
  [[nodiscard]] auto
  resolve_package(std::string_view name,
                  std::optional<PluginVersion> version) const -> std::optional<RegistryPackage>;

  /// Write a synthetic manifest.toml for an installed package
  [[nodiscard]] auto
  write_installed_manifest(const RegistryPackage& pkg) const -> tl::expected<void, RegistryError>;
};

} // namespace horcrux::core
