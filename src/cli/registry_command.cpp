// Horcrux - Registry Management Commands Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "registry_command.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

#include "../core/registry_client.h"

namespace horcrux::cli {

// ─────────────────────────────────────────────────────────────────────────────
// RegistryCommandError to_string
// ─────────────────────────────────────────────────────────────────────────────

auto to_string(RegistryCommandError error) -> std::string {
  switch (error) {
  case RegistryCommandError::NoSubcommand:
    return "No registry subcommand provided";
  case RegistryCommandError::InvalidSubcommand:
    return "Invalid registry subcommand";
  case RegistryCommandError::MissingArgument:
    return "Missing required argument";
  case RegistryCommandError::OperationFailed:
    return "Registry operation failed";
  }
  return "Unknown registry command error";
}

// ─────────────────────────────────────────────────────────────────────────────
// Usage
// ─────────────────────────────────────────────────────────────────────────────

namespace {

void print_registry_usage() {
  std::cout << "Usage: horcrux registry <subcommand> [options]\n\n";
  std::cout << "Manage Horcrux plugin registries.\n\n";
  std::cout << "Subcommands:\n";
  std::cout << "  add <name> <url> [--trusted]  Add a registry\n";
  std::cout << "  remove <name>                 Remove a registry\n";
  std::cout << "  list                          List configured registries\n\n";
  std::cout << "Options:\n";
  std::cout << "  --config=FILE  Registry config file "
               "(default: ~/.horcrux/registries.conf)\n";
  std::cout << "  --verbose, -v  Enable verbose logging\n\n";
  std::cout << "Examples:\n";
  std::cout << "  horcrux registry add official https://registry.horcrux.dev\n";
  std::cout << "  horcrux registry add official https://registry.horcrux.dev --trusted\n";
  std::cout << "  horcrux registry list\n";
  std::cout << "  horcrux registry remove official\n";
}

/// Default registry config path
auto default_registry_config() -> std::filesystem::path {
  const char* home = std::getenv("HOME"); // NOLINT(concurrency-mt-unsafe)
  if (home != nullptr) {
    return std::filesystem::path(home) / ".horcrux" / "registries.conf";
  }
  return std::filesystem::path(".horcrux") / "registries.conf";
}

/// Persist a list of registry configs to a file
auto save_registries(const std::filesystem::path& config_path,
                     const std::vector<core::RegistryConfig>& registries) -> bool {
  auto parent = config_path.parent_path();
  if (!parent.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      return false;
    }
  }

  std::ofstream file(config_path);
  if (!file) {
    return false;
  }
  file << "# Horcrux Registry Configuration\n";
  for (const auto& r : registries) {
    file << "name=" << r.name << " url=" << r.url << " trusted=" << (r.trusted ? "true" : "false")
         << "\n";
  }
  return true;
}

/// Load registry configs from a file
auto load_registries(const std::filesystem::path& config_path)
    -> std::vector<core::RegistryConfig> {
  std::vector<core::RegistryConfig> result;
  if (!std::filesystem::exists(config_path)) {
    return result;
  }
  std::ifstream file(config_path);
  if (!file) {
    return result;
  }

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line.starts_with('#')) {
      continue;
    }
    core::RegistryConfig cfg;
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
        cfg.name = val;
      } else if (key == "url") {
        cfg.url = val;
      } else if (key == "trusted") {
        cfg.trusted = (val == "true");
      }
    }
    if (!cfg.name.empty() && !cfg.url.empty()) {
      result.push_back(std::move(cfg));
    }
  }
  return result;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// handle_registry_command
// ─────────────────────────────────────────────────────────────────────────────

auto handle_registry_command(int argc, char* argv[], Logger& logger) -> int {
  if (argc < 3) {
    print_registry_usage();
    return 1;
  }

  std::string_view subcmd = argv[2];

  if (subcmd == "--help" || subcmd == "-h" || subcmd == "help") {
    print_registry_usage();
    return 0;
  }

  // Parse global options
  std::filesystem::path config_path = default_registry_config();
  for (int i = 3; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg.starts_with("--config=")) {
      config_path = arg.substr(9);
    } else if (arg == "--verbose" || arg == "-v") {
      logger.set_level(LogLevel::Debug);
    }
  }

  auto registries = load_registries(config_path);

  if (subcmd == "list") {
    if (registries.empty()) {
      logger.info("No registries configured.");
      return 0;
    }
    std::cout << "Configured registries:\n";
    for (const auto& r : registries) {
      std::cout << "  " << r.name << "  " << r.url;
      if (r.trusted) {
        std::cout << "  [trusted]";
      }
      std::cout << "\n";
    }
    return 0;
  }

  if (subcmd == "add") {
    if (argc < 5) {
      logger.error("Usage: horcrux registry add <name> <url> [--trusted]");
      return 1;
    }
    std::string name = argv[3];
    std::string url = argv[4];
    bool trusted = false;
    for (int i = 5; i < argc; ++i) {
      if (std::string_view(argv[i]) == "--trusted") {
        trusted = true;
      }
    }

    // Check for duplicate
    for (const auto& r : registries) {
      if (r.name == name) {
        logger.error("Registry '", name, "' is already configured");
        return 1;
      }
    }

    registries.push_back({.name = name, .url = url, .trusted = trusted});
    if (!save_registries(config_path, registries)) {
      logger.error("Failed to save registry configuration");
      return 1;
    }
    logger.info("Added registry '", name, "' -> ", url);
    return 0;
  }

  if (subcmd == "remove") {
    if (argc < 4) {
      logger.error("Usage: horcrux registry remove <name>");
      return 1;
    }
    std::string name = argv[3];
    auto before = registries.size();
    registries.erase(
        std::remove_if(registries.begin(), registries.end(),
                       [&name](const core::RegistryConfig& r) { return r.name == name; }),
        registries.end());
    if (registries.size() == before) {
      logger.error("Registry '", name, "' not found");
      return 1;
    }
    if (!save_registries(config_path, registries)) {
      logger.error("Failed to save registry configuration");
      return 1;
    }
    logger.info("Removed registry '", name, "'");
    return 0;
  }

  logger.error("Unknown registry subcommand: ", subcmd);
  std::cerr << "Run 'horcrux registry --help' for usage.\n";
  return 1;
}

} // namespace horcrux::cli
