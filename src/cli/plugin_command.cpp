// Horcrux - Plugin Management Commands Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "plugin_command.h"

#include <filesystem>
#include <iostream>
#include <string_view>

#include "../core/plugin_loader.h"
#include "../core/registry_client.h"

namespace horcrux::cli {

// ─────────────────────────────────────────────────────────────────────────────
// PluginCommandError to_string
// ─────────────────────────────────────────────────────────────────────────────

auto to_string(PluginCommandError error) -> std::string {
  switch (error) {
  case PluginCommandError::NoSubcommand:
    return "No plugin subcommand provided";
  case PluginCommandError::InvalidSubcommand:
    return "Invalid plugin subcommand";
  case PluginCommandError::MissingArgument:
    return "Missing required argument";
  case PluginCommandError::OperationFailed:
    return "Plugin operation failed";
  }
  return "Unknown plugin command error";
}

// ─────────────────────────────────────────────────────────────────────────────
// Usage
// ─────────────────────────────────────────────────────────────────────────────

namespace {

void print_plugin_usage() {
  std::cout << "Usage: horcrux plugin <subcommand> [options]\n\n";
  std::cout << "Manage Horcrux plugins.\n\n";
  std::cout << "Subcommands:\n";
  std::cout << "  search <query>            Search registry for plugins\n";
  std::cout << "  install <name> [version]  Install a plugin from registry\n";
  std::cout << "  list                      List installed plugins\n";
  std::cout << "  info <name>               Show plugin details\n";
  std::cout << "  update <name> [version]   Update an installed plugin\n";
  std::cout << "  remove <name>             Remove an installed plugin\n\n";
  std::cout << "Options:\n";
  std::cout << "  --plugins-dir=DIR         Plugin install directory "
               "(default: ~/.horcrux/plugins)\n";
  std::cout << "  --lockfile=FILE           Lockfile path "
               "(default: ~/.horcrux/plugins.lock)\n";
  std::cout << "  --verbose, -v             Enable verbose logging\n\n";
  std::cout << "Examples:\n";
  std::cout << "  horcrux plugin search wasm\n";
  std::cout << "  horcrux plugin install horcrux-wasm\n";
  std::cout << "  horcrux plugin install horcrux-wasm 1.2.0\n";
  std::cout << "  horcrux plugin list\n";
  std::cout << "  horcrux plugin info horcrux-wasm\n";
  std::cout << "  horcrux plugin update horcrux-wasm\n";
  std::cout << "  horcrux plugin remove horcrux-wasm\n";
}

/// Default plugins directory: ~/.horcrux/plugins
auto default_plugins_dir() -> std::filesystem::path {
  const char* home = std::getenv("HOME"); // NOLINT(concurrency-mt-unsafe)
  if (home != nullptr) {
    return std::filesystem::path(home) / ".horcrux" / "plugins";
  }
  return std::filesystem::path(".horcrux") / "plugins";
}

/// Default lockfile: ~/.horcrux/plugins.lock
auto default_lockfile_path() -> std::filesystem::path {
  const char* home = std::getenv("HOME"); // NOLINT(concurrency-mt-unsafe)
  if (home != nullptr) {
    return std::filesystem::path(home) / ".horcrux" / "plugins.lock";
  }
  return std::filesystem::path(".horcrux") / "plugins.lock";
}

// ─────────────────────────────────────────────────────────────────────────────
// Subcommand handlers
// ─────────────────────────────────────────────────────────────────────────────

auto cmd_search(const std::string& query, core::RegistryClient& client,
                Logger& logger) -> int {
  auto results = client.search(query);
  if (results.empty()) {
    logger.info("No plugins found matching: ", query.empty() ? "(all)" : query);
    return 0;
  }
  std::cout << "Found " << results.size() << " plugin(s):\n";
  for (const auto& pkg : results) {
    std::cout << "  " << pkg.name << " v" << pkg.version.to_string() << " - "
              << pkg.description << " [" << pkg.license << "]\n";
  }
  return 0;
}

auto cmd_install(const std::string& name,
                  std::optional<core::PluginVersion> version,
                  core::RegistryClient& client,
                  Logger& logger) -> int {
  logger.info("Installing plugin: ", name);
  auto result = client.install(name, version, /*prompt_trust=*/true);
  if (!result) {
    logger.error("Install failed: ", core::to_string(result.error()));
    return 1;
  }
  logger.info("Installed ", result->name, " v", result->version.to_string());
  return 0;
}

auto cmd_list(core::RegistryClient& client, Logger& logger) -> int {
  auto installed = client.list_installed();
  if (installed.empty()) {
    logger.info("No plugins installed.");
    return 0;
  }
  std::cout << "Installed plugins:\n";
  for (const auto& entry : installed) {
    std::cout << "  " << entry.name << " v" << entry.version.to_string();
    if (!entry.checksum.empty()) {
      std::cout << " (sha256: " << entry.checksum.substr(0, 12) << "...)";
    }
    std::cout << "\n";
  }
  return 0;
}

auto cmd_info(const std::string& name, core::RegistryClient& client,
               Logger& logger) -> int {
  auto info = client.package_info(name);
  if (!info.has_value()) {
    logger.error("Plugin not found in registry: ", name);
    return 1;
  }
  std::cout << "Name:        " << info->name << "\n";
  std::cout << "Version:     " << info->version.to_string() << "\n";
  std::cout << "Author:      " << info->author << "\n";
  std::cout << "License:     " << info->license << "\n";
  std::cout << "Description: " << info->description << "\n";
  if (!info->checksum.empty()) {
    std::cout << "Checksum:    " << info->checksum << "\n";
  }
  return 0;
}

auto cmd_update(const std::string& name,
                 std::optional<core::PluginVersion> version,
                 core::RegistryClient& client,
                 Logger& logger) -> int {
  logger.info("Updating plugin: ", name);
  auto result = client.update(name, version);
  if (!result) {
    logger.error("Update failed: ", core::to_string(result.error()));
    return 1;
  }
  logger.info("Updated ", result->name, " to v", result->version.to_string());
  return 0;
}

auto cmd_remove(const std::string& name, core::RegistryClient& client,
                 Logger& logger) -> int {
  logger.info("Removing plugin: ", name);
  auto result = client.remove(name);
  if (!result) {
    logger.error("Remove failed: ", core::to_string(result.error()));
    return 1;
  }
  logger.info("Removed plugin: ", name);
  return 0;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// handle_plugin_command
// ─────────────────────────────────────────────────────────────────────────────

auto handle_plugin_command(int argc, char* argv[], Logger& logger) -> int {
  if (argc < 3) {
    print_plugin_usage();
    return 1;
  }

  std::string_view subcmd = argv[2];

  if (subcmd == "--help" || subcmd == "-h" || subcmd == "help") {
    print_plugin_usage();
    return 0;
  }

  // Parse global options
  std::filesystem::path plugins_dir = default_plugins_dir();
  std::filesystem::path lockfile_path = default_lockfile_path();

  for (int i = 3; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg.starts_with("--plugins-dir=")) {
      plugins_dir = arg.substr(14);
    } else if (arg.starts_with("--lockfile=")) {
      lockfile_path = arg.substr(11);
    } else if (arg == "--verbose" || arg == "-v") {
      logger.set_level(LogLevel::Debug);
    }
  }

  core::RegistryClient client(plugins_dir, lockfile_path);

  if (subcmd == "search") {
    std::string query;
    if (argc > 3) {
      // Collect all non-flag arguments as the query
      for (int i = 3; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (!arg.starts_with("--")) {
          if (!query.empty()) {
            query += " ";
          }
          query += std::string(arg);
        }
      }
    }
    return cmd_search(query, client, logger);
  }

  if (subcmd == "list") {
    return cmd_list(client, logger);
  }

  if (subcmd == "install") {
    if (argc < 4) {
      logger.error("Missing plugin name");
      std::cerr << "Usage: horcrux plugin install <name> [version]\n";
      return 1;
    }
    std::string name = argv[3];
    std::optional<core::PluginVersion> version;
    if (argc >= 5) {
      std::string_view v = argv[4];
      if (!v.starts_with("--")) {
        auto parsed = core::PluginVersion::parse(v);
        if (parsed) {
          version = *parsed;
        }
      }
    }
    return cmd_install(name, version, client, logger);
  }

  if (subcmd == "info") {
    if (argc < 4) {
      logger.error("Missing plugin name");
      std::cerr << "Usage: horcrux plugin info <name>\n";
      return 1;
    }
    return cmd_info(std::string(argv[3]), client, logger);
  }

  if (subcmd == "update") {
    if (argc < 4) {
      logger.error("Missing plugin name");
      std::cerr << "Usage: horcrux plugin update <name> [version]\n";
      return 1;
    }
    std::string name = argv[3];
    std::optional<core::PluginVersion> version;
    if (argc >= 5) {
      std::string_view v = argv[4];
      if (!v.starts_with("--")) {
        auto parsed = core::PluginVersion::parse(v);
        if (parsed) {
          version = *parsed;
        }
      }
    }
    return cmd_update(name, version, client, logger);
  }

  if (subcmd == "remove") {
    if (argc < 4) {
      logger.error("Missing plugin name");
      std::cerr << "Usage: horcrux plugin remove <name>\n";
      return 1;
    }
    return cmd_remove(std::string(argv[3]), client, logger);
  }

  logger.error("Unknown plugin subcommand: ", subcmd);
  std::cerr << "Run 'horcrux plugin --help' for usage.\n";
  return 1;
}

} // namespace horcrux::cli
