// Horcrux - Plugin Management Commands
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <string>

#include <tl/expected.hpp>

#include "logger.h"

namespace horcrux::cli {

/// @brief Error types for plugin commands
enum class PluginCommandError {
  NoSubcommand,      ///< No subcommand was provided
  InvalidSubcommand, ///< Unrecognized subcommand
  MissingArgument,   ///< Required argument (e.g., plugin name) is missing
  OperationFailed,   ///< The underlying registry/loader operation failed
};

/// @brief Convert PluginCommandError to human-readable string
[[nodiscard]] auto to_string(PluginCommandError error) -> std::string;

/// @brief Handle the `horcrux plugin` command family
///
/// Subcommands:
///   plugin search <query>           Search registry for plugins
///   plugin install <name> [version] Install a plugin
///   plugin list                     List installed plugins
///   plugin info <name>              Show plugin details
///   plugin update <name> [version]  Update a plugin
///   plugin remove <name>            Remove a plugin
///
/// @param argc Argument count
/// @param argv Argument vector
/// @param logger Logger instance
/// @return Exit code (0 = success)
auto handle_plugin_command(int argc, char* argv[], Logger& logger) -> int;

} // namespace horcrux::cli
