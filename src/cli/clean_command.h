// Horcrux - Clean Command
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <string>

#include <tl/expected.hpp>

#include "logger.h"

namespace horcrux::cli {

/// @brief Scope of the clean operation
enum class CleanScope {
  All,     ///< Remove both build outputs and cache
  Outputs, ///< Remove only build outputs (default output directory)
  Cache,   ///< Remove only the local cache directory
};

/// @brief Options for the clean command
struct CleanOptions {
  CleanScope scope = CleanScope::All;
  std::filesystem::path output_dir = "horcrux-out";
  std::filesystem::path cache_dir = ".horcrux-cache";
  bool dry_run = false; ///< Show what would be removed without removing it
  bool verbose = false;
};

/// @brief Error types for the clean command
enum class CleanError {
  InvalidPath,   ///< Provided path is unsafe or invalid
  RemovalFailed, ///< Failed to remove a file or directory
};

/// @brief Convert CleanError to human-readable string
[[nodiscard]] auto to_string(CleanError error) -> std::string;

/// @brief Validate a path to prevent accidental removal of system directories
/// @param path The path to validate
/// @return true if the path is safe to remove, false otherwise
[[nodiscard]] auto is_safe_path(const std::filesystem::path& path) -> bool;

/// @brief Handle the `horcrux clean` command
/// @param argc Argument count
/// @param argv Argument vector
/// @param logger Logger instance
/// @return Exit code (0 = success)
auto handle_clean_command(int argc, char* argv[], Logger& logger) -> int;

} // namespace horcrux::cli
