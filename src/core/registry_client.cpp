// Horcrux - Registry Client Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "registry_client.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace horcrux::core {

// ─────────────────────────────────────────────────────────────────────────────
// RegistryError to_string
// ─────────────────────────────────────────────────────────────────────────────

auto to_string(RegistryError error) -> std::string {
  switch (error) {
  case RegistryError::NetworkUnavailable:
    return "Registry network unavailable";
  case RegistryError::PackageNotFound:
    return "Package not found in registry";
  case RegistryError::VersionNotFound:
    return "Requested version not found in registry";
  case RegistryError::DownloadFailed:
    return "Package download failed";
  case RegistryError::ChecksumMismatch:
    return "Downloaded package checksum mismatch";
  case RegistryError::InstallFailed:
    return "Package installation failed";
  case RegistryError::AlreadyInstalled:
    return "Package is already installed at this version";
  case RegistryError::LockfileError:
    return "Failed to read or write plugin lockfile";
  case RegistryError::InvalidConfig:
    return "Registry configuration is invalid";
  }
  return "Unknown registry error";
}

// ─────────────────────────────────────────────────────────────────────────────
// PluginLockfile
// ─────────────────────────────────────────────────────────────────────────────

auto PluginLockfile::find(std::string_view name) const -> const LockfileEntry* {
  for (const auto& entry : entries) {
    if (entry.name == name) {
      return &entry;
    }
  }
  return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// RegistryClient
// ─────────────────────────────────────────────────────────────────────────────

RegistryClient::RegistryClient(std::filesystem::path plugins_dir,
                                std::filesystem::path lockfile_path)
    : plugins_dir_(std::move(plugins_dir)),
      lockfile_path_(std::move(lockfile_path)) {}

// ── Registry management ──────────────────────────────────────────────────────

void RegistryClient::add_registry(RegistryConfig config) {
  registries_.push_back(std::move(config));
}

auto RegistryClient::remove_registry(std::string_view name) -> bool {
  auto it = std::remove_if(registries_.begin(), registries_.end(),
                            [name](const RegistryConfig& r) { return r.name == name; });
  if (it == registries_.end()) {
    return false;
  }
  registries_.erase(it, registries_.end());
  return true;
}

auto RegistryClient::list_registries() const -> const std::vector<RegistryConfig>& {
  return registries_;
}

// ── Package index ─────────────────────────────────────────────────────────────

void RegistryClient::seed_package(RegistryPackage package) {
  index_.push_back(std::move(package));
}

auto RegistryClient::search(std::string_view query) const -> std::vector<RegistryPackage> {
  std::vector<RegistryPackage> results;

  auto to_lower = [](std::string s) -> std::string {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
  };

  const std::string q = to_lower(std::string(query));

  for (const auto& pkg : index_) {
    const std::string name_lower = to_lower(pkg.name);
    const std::string desc_lower = to_lower(pkg.description);

    if (q.empty() || name_lower.find(q) != std::string::npos ||
        desc_lower.find(q) != std::string::npos) {
      results.push_back(pkg);
    }
  }
  return results;
}

auto RegistryClient::package_info(std::string_view name) const
    -> std::optional<RegistryPackage> {
  for (const auto& pkg : index_) {
    if (pkg.name == name) {
      return pkg;
    }
  }
  return std::nullopt;
}

// ── Internal helpers ──────────────────────────────────────────────────────────

auto RegistryClient::resolve_package(std::string_view name,
                                      std::optional<PluginVersion> version) const
    -> std::optional<RegistryPackage> {
  std::optional<RegistryPackage> best;
  for (const auto& pkg : index_) {
    if (pkg.name != name) {
      continue;
    }
    if (version.has_value() && pkg.version != *version) {
      continue;
    }
    // Pick latest version
    if (!best.has_value() || pkg.version > best->version) {
      best = pkg;
    }
  }
  return best;
}

auto RegistryClient::write_installed_manifest(const RegistryPackage& pkg) const
    -> tl::expected<void, RegistryError> {
  // Safety: reject names containing path separators or parent traversal
  if (pkg.name.find('/') != std::string::npos ||
      pkg.name.find('\\') != std::string::npos || pkg.name.find("..") != std::string::npos ||
      pkg.name.empty()) {
    return tl::unexpected(RegistryError::InstallFailed);
  }

  auto plugin_dir = plugins_dir_ / pkg.name;
  std::error_code ec;
  std::filesystem::create_directories(plugin_dir, ec);
  if (ec) {
    return tl::unexpected(RegistryError::InstallFailed);
  }

  std::ofstream file(plugin_dir / "manifest.toml");
  if (!file) {
    return tl::unexpected(RegistryError::InstallFailed);
  }

  file << "name = \"" << pkg.name << "\"\n";
  file << "version = \"" << pkg.version.to_string() << "\"\n";
  file << "author = \"" << pkg.author << "\"\n";
  file << "license = \"" << pkg.license << "\"\n";
  file << "description = \"" << pkg.description << "\"\n";
  if (!pkg.checksum.empty()) {
    file << "checksum = \"" << pkg.checksum << "\"\n";
  }

  return {};
}

// ── Install / update / remove ────────────────────────────────────────────────

auto RegistryClient::install(std::string_view name,
                              std::optional<PluginVersion> version,
                              bool prompt_trust)
    -> tl::expected<LockfileEntry, RegistryError> {
  auto pkg = resolve_package(name, version);
  if (!pkg.has_value()) {
    if (version.has_value()) {
      return tl::unexpected(RegistryError::VersionNotFound);
    }
    return tl::unexpected(RegistryError::PackageNotFound);
  }

  // Check if already installed at the same version
  auto installed = list_installed();
  for (const auto& entry : installed) {
    if (entry.name == pkg->name && entry.version == pkg->version) {
      return tl::unexpected(RegistryError::AlreadyInstalled);
    }
  }

  // Trust prompt (printed to stdout; in real CLI this would wait for input)
  if (prompt_trust) {
    std::cout << "Installing plugin '" << pkg->name << "' v" << pkg->version.to_string()
              << " from registry. Do you trust this plugin? [y/N]\n";
  }

  // Simulate download + installation (write manifest to plugins dir)
  auto write_result = write_installed_manifest(*pkg);
  if (!write_result) {
    return tl::unexpected(write_result.error());
  }

  LockfileEntry entry{
      .name = pkg->name,
      .version = pkg->version,
      .checksum = pkg->checksum,
      .registry_url = pkg->download_url,
  };

  // Update lockfile
  auto lockfile = read_lockfile();
  lockfile.entries.push_back(entry);
  auto write_lock = write_lockfile(lockfile);
  if (!write_lock) {
    return tl::unexpected(write_lock.error());
  }

  return entry;
}

auto RegistryClient::update(std::string_view name,
                             std::optional<PluginVersion> version)
    -> tl::expected<LockfileEntry, RegistryError> {
  // Safety: reject names containing path separators or parent traversal
  const std::string name_str(name);
  if (name_str.find('/') != std::string::npos ||
      name_str.find('\\') != std::string::npos ||
      name_str.find("..") != std::string::npos || name_str.empty()) {
    return tl::unexpected(RegistryError::PackageNotFound);
  }

  // Remove existing installation if present
  auto plugin_dir = plugins_dir_ / name_str;
  if (std::filesystem::exists(plugin_dir)) {
    std::error_code ec;
    std::filesystem::remove_all(plugin_dir, ec);
  }

  // Remove from lockfile
  auto lockfile = read_lockfile();
  lockfile.entries.erase(
      std::remove_if(lockfile.entries.begin(), lockfile.entries.end(),
                     [&name_str](const LockfileEntry& e) { return e.name == name_str; }),
      lockfile.entries.end());

  // Re-install
  auto pkg = resolve_package(name, version);
  if (!pkg.has_value()) {
    return tl::unexpected(version.has_value() ? RegistryError::VersionNotFound
                                              : RegistryError::PackageNotFound);
  }

  auto write_result = write_installed_manifest(*pkg);
  if (!write_result) {
    return tl::unexpected(write_result.error());
  }

  LockfileEntry entry{
      .name = pkg->name,
      .version = pkg->version,
      .checksum = pkg->checksum,
      .registry_url = pkg->download_url,
  };

  lockfile.entries.push_back(entry);
  auto write_lock = write_lockfile(lockfile);
  if (!write_lock) {
    return tl::unexpected(write_lock.error());
  }

  return entry;
}

auto RegistryClient::remove(std::string_view name) -> tl::expected<void, RegistryError> {
  // Safety: reject names containing path separators or parent traversal
  const std::string name_str(name);
  if (name_str.find('/') != std::string::npos ||
      name_str.find('\\') != std::string::npos ||
      name_str.find("..") != std::string::npos || name_str.empty()) {
    return tl::unexpected(RegistryError::PackageNotFound);
  }

  auto plugin_dir = plugins_dir_ / name_str;
  if (!std::filesystem::exists(plugin_dir)) {
    return tl::unexpected(RegistryError::PackageNotFound);
  }

  std::error_code ec;
  std::filesystem::remove_all(plugin_dir, ec);
  if (ec) {
    return tl::unexpected(RegistryError::InstallFailed);
  }

  // Update lockfile
  auto lockfile = read_lockfile();
  lockfile.entries.erase(
      std::remove_if(lockfile.entries.begin(), lockfile.entries.end(),
                     [&name_str](const LockfileEntry& e) { return e.name == name_str; }),
      lockfile.entries.end());

  return write_lockfile(lockfile);
}

// ── Lockfile ─────────────────────────────────────────────────────────────────

auto RegistryClient::read_lockfile() const -> PluginLockfile {
  PluginLockfile lockfile;
  if (!std::filesystem::exists(lockfile_path_)) {
    return lockfile;
  }

  std::ifstream file(lockfile_path_);
  if (!file) {
    return lockfile;
  }

  // Simple line-oriented format:
  // name=<name> version=<ver> checksum=<sha> registry=<url>
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line.starts_with('#')) {
      continue;
    }
    LockfileEntry entry;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
      auto eq = token.find('=');
      if (eq == std::string::npos) {
        continue;
      }
      auto key = token.substr(0, eq);
      auto val = token.substr(eq + 1);
      if (key == "name") {
        entry.name = val;
      } else if (key == "version") {
        auto v = PluginVersion::parse(val);
        if (v) {
          entry.version = *v;
        }
      } else if (key == "checksum") {
        entry.checksum = val;
      } else if (key == "registry") {
        entry.registry_url = val;
      }
    }
    if (!entry.name.empty()) {
      lockfile.entries.push_back(std::move(entry));
    }
  }
  return lockfile;
}

auto RegistryClient::write_lockfile(const PluginLockfile& lockfile) const
    -> tl::expected<void, RegistryError> {
  // Ensure parent directory exists
  auto parent = lockfile_path_.parent_path();
  if (!parent.empty() && !std::filesystem::exists(parent)) {
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      return tl::unexpected(RegistryError::LockfileError);
    }
  }

  std::ofstream file(lockfile_path_);
  if (!file) {
    return tl::unexpected(RegistryError::LockfileError);
  }

  file << "# Horcrux Plugin Lockfile - do not edit manually\n";
  for (const auto& entry : lockfile.entries) {
    file << "name=" << entry.name << " version=" << entry.version.to_string()
         << " checksum=" << entry.checksum << " registry=" << entry.registry_url << "\n";
  }
  return {};
}

auto RegistryClient::list_installed() const -> std::vector<LockfileEntry> {
  auto lockfile = read_lockfile();
  return lockfile.entries;
}

} // namespace horcrux::core
