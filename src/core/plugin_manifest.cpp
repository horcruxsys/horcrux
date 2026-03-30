// Horcrux - Plugin Manifest Parser Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "plugin_manifest.h"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <string>

namespace horcrux::core {

// ─────────────────────────────────────────────────────────────────────────────
// PluginVersion
// ─────────────────────────────────────────────────────────────────────────────

auto PluginVersion::to_string() const -> std::string {
  return std::to_string(ver_major) + "." + std::to_string(ver_minor) + "." +
         std::to_string(ver_patch);
}

auto PluginVersion::parse(std::string_view s) -> tl::expected<PluginVersion, std::string> {
  PluginVersion v;
  auto parse_u32 = [](std::string_view part, uint32_t& out) -> bool {
    auto result = std::from_chars(part.data(), part.data() + part.size(), out);
    return result.ec == std::errc{} && result.ptr == part.data() + part.size();
  };

  auto dot1 = s.find('.');
  if (dot1 == std::string_view::npos) {
    return tl::unexpected<std::string>("Missing '.' separator in version: " + std::string(s));
  }
  auto dot2 = s.find('.', dot1 + 1);
  if (dot2 == std::string_view::npos) {
    return tl::unexpected<std::string>("Missing second '.' separator in version: " +
                                       std::string(s));
  }

  if (!parse_u32(s.substr(0, dot1), v.ver_major) ||
      !parse_u32(s.substr(dot1 + 1, dot2 - dot1 - 1), v.ver_minor) ||
      !parse_u32(s.substr(dot2 + 1), v.ver_patch)) {
    return tl::unexpected<std::string>("Non-numeric version component in: " + std::string(s));
  }
  return v;
}

// ─────────────────────────────────────────────────────────────────────────────
// ManifestError to_string
// ─────────────────────────────────────────────────────────────────────────────

auto to_string(ManifestError error) -> std::string {
  switch (error) {
  case ManifestError::ParseError:
    return "Manifest parse error";
  case ManifestError::MissingField:
    return "Required manifest field is missing";
  case ManifestError::InvalidVersion:
    return "Invalid version string in manifest";
  case ManifestError::InvalidLicense:
    return "Invalid or unrecognized license identifier";
  case ManifestError::InvalidExtension:
    return "Malformed extension entry in manifest";
  }
  return "Unknown manifest error";
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Trim leading/trailing whitespace from a string_view
auto trim(std::string_view s) -> std::string_view {
  const auto start = s.find_first_not_of(" \t\r\n");
  if (start == std::string_view::npos) {
    return {};
  }
  const auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

/// Strip surrounding double-quotes from a value token
auto unquote(std::string_view s) -> std::string_view {
  if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
    return s.substr(1, s.size() - 2);
  }
  return s;
}

/// Parse a bool token ("true" → true, "false" → false)
auto parse_bool(std::string_view s) -> std::optional<bool> {
  if (s == "true") {
    return true;
  }
  if (s == "false") {
    return false;
  }
  return std::nullopt;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// parse_plugin_manifest
// ─────────────────────────────────────────────────────────────────────────────

auto parse_plugin_manifest(std::string_view text)
    -> tl::expected<PluginManifest, ManifestError> {
  PluginManifest manifest;
  PluginExtension current_extension;
  bool in_extension_block = false;

  std::istringstream stream{std::string(text)};
  std::string line_str;

  while (std::getline(stream, line_str)) {
    std::string_view line = trim(line_str);

    // Skip blank lines and comments
    if (line.empty() || line.starts_with('#')) {
      continue;
    }

    // Section header: [extension]
    if (line == "[extension]") {
      // Flush previous extension block if populated
      if (in_extension_block && !current_extension.kind.empty() &&
          !current_extension.name.empty()) {
        manifest.extensions.push_back(current_extension);
      }
      current_extension = {};
      in_extension_block = true;
      continue;
    }

    // Key = value parsing
    auto eq = line.find('=');
    if (eq == std::string_view::npos) {
      continue; // Ignore lines without '='
    }

    std::string_view key = trim(line.substr(0, eq));
    std::string_view val = trim(unquote(trim(line.substr(eq + 1))));

    if (in_extension_block) {
      if (key == "kind") {
        current_extension.kind = std::string(val);
      } else if (key == "name") {
        current_extension.name = std::string(val);
      }
      continue;
    }

    // Top-level fields
    if (key == "name") {
      manifest.name = std::string(val);
    } else if (key == "version") {
      auto v = PluginVersion::parse(val);
      if (!v) {
        return tl::unexpected(ManifestError::InvalidVersion);
      }
      manifest.version = *v;
    } else if (key == "author") {
      manifest.author = std::string(val);
    } else if (key == "license") {
      manifest.license = std::string(val);
    } else if (key == "description") {
      manifest.description = std::string(val);
    } else if (key == "min_horcrux_version") {
      auto v = PluginVersion::parse(val);
      if (!v) {
        return tl::unexpected(ManifestError::InvalidVersion);
      }
      manifest.min_horcrux_version = *v;
    } else if (key == "max_horcrux_version") {
      auto v = PluginVersion::parse(val);
      if (!v) {
        return tl::unexpected(ManifestError::InvalidVersion);
      }
      manifest.max_horcrux_version = *v;
    } else if (key == "checksum") {
      manifest.checksum = std::string(val);
    } else if (key == "permissions.filesystem_read") {
      auto b = parse_bool(val);
      if (b) {
        manifest.permissions.filesystem_read = *b;
      }
    } else if (key == "permissions.filesystem_write") {
      auto b = parse_bool(val);
      if (b) {
        manifest.permissions.filesystem_write = *b;
      }
    } else if (key == "permissions.network_access") {
      auto b = parse_bool(val);
      if (b) {
        manifest.permissions.network_access = *b;
      }
    } else if (key == "permissions.process_spawn") {
      auto b = parse_bool(val);
      if (b) {
        manifest.permissions.process_spawn = *b;
      }
    }
  }

  // Flush last extension block
  if (in_extension_block && !current_extension.kind.empty() &&
      !current_extension.name.empty()) {
    manifest.extensions.push_back(current_extension);
  }

  // Validate required fields
  if (manifest.name.empty()) {
    return tl::unexpected(ManifestError::MissingField);
  }
  if (manifest.version.to_string() == "0.0.0") {
    // Allow 0.0.0 as a valid explicit version; only reject if never set
    // (name check above is the primary guard)
  }
  if (manifest.license.empty()) {
    return tl::unexpected(ManifestError::MissingField);
  }

  return manifest;
}

// ─────────────────────────────────────────────────────────────────────────────
// validate_plugin_manifest
// ─────────────────────────────────────────────────────────────────────────────

auto validate_plugin_manifest(const PluginManifest& manifest,
                               const PluginVersion& horcrux_version)
    -> std::optional<std::string> {
  // Check compatibility range
  if (horcrux_version < manifest.min_horcrux_version) {
    return "Plugin '" + manifest.name + "' requires Horcrux >= " +
           manifest.min_horcrux_version.to_string() + " (current: " +
           horcrux_version.to_string() + ")";
  }

  // Only check max if it was explicitly set (non-zero)
  if (manifest.max_horcrux_version.ver_major > 0 ||
      manifest.max_horcrux_version.ver_minor > 0 ||
      manifest.max_horcrux_version.ver_patch > 0) {
    if (horcrux_version > manifest.max_horcrux_version) {
      return "Plugin '" + manifest.name + "' is only compatible up to Horcrux " +
             manifest.max_horcrux_version.to_string() + " (current: " +
             horcrux_version.to_string() + ")";
    }
  }

  // Validate extensions
  for (const auto& ext : manifest.extensions) {
    if (ext.kind.empty() || ext.name.empty()) {
      return "Plugin '" + manifest.name + "' has an extension entry with empty kind or name";
    }
    if (ext.kind != "rule" && ext.kind != "adapter" && ext.kind != "toolchain") {
      return "Plugin '" + manifest.name + "' has unknown extension kind: " + ext.kind;
    }
  }

  return std::nullopt;
}

} // namespace horcrux::core
