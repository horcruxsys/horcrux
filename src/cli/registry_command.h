// Horcrux - Registry Management Commands
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <string>

#include <tl/expected.hpp>

#include "logger.h"

namespace horcrux::cli {

/// @brief Error types for registry management commands
enum class RegistryCommandError {
  NoSubcommand,      ///< No subcommand was provided
  InvalidSubcommand, ///< Unrecognized subcommand
  MissingArgument,   ///< Required argument is missing
  OperationFailed,   ///< The underlying operation failed
};

/// @brief Convert RegistryCommandError to human-readable string
[[nodiscard]] auto to_string(RegistryCommandError error) -> std::string;

/// @brief Handle the `horcrux registry` command family
///
/// Subcommands:
///   registry add <name> <url> [--trusted]  Add a registry
///   registry remove <name>                 Remove a registry
///   registry list                          List configured registries
///
/// @param argc Argument count
/// @param argv Argument vector
/// @param logger Logger instance
/// @return Exit code (0 = success)
auto handle_registry_command(int argc, char* argv[], Logger& logger) -> int;

} // namespace horcrux::cli
