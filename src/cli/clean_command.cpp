// Horcrux - Clean Command Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "clean_command.h"

#include <iostream>
#include <string_view>
#include <system_error>

namespace horcrux::cli {

auto to_string(CleanError error) -> std::string {
  switch (error) {
  case CleanError::InvalidPath:
    return "Invalid or unsafe path";
  case CleanError::RemovalFailed:
    return "Failed to remove directory";
  }
  return "Unknown clean error";
}

constexpr int MIN_SAFE_ABSOLUTE_PATH_DEPTH = 3;

void print_clean_usage() {
  std::cout << "Usage: horcrux clean [options]\n\n";
  std::cout << "Remove build outputs and/or local cache artifacts.\n\n";
  std::cout << "Options:\n";
  std::cout << "  --outputs         Remove build output directory only\n";
  std::cout << "  --cache           Remove local cache directory only\n";
  std::cout << "  --all             Remove both outputs and cache (default)\n";
  std::cout << "  --output-dir=DIR  Build output directory (default: horcrux-out)\n";
  std::cout << "  --cache-dir=DIR   Cache directory (default: .horcrux-cache)\n";
  std::cout << "  --dry-run         Show what would be removed without removing it\n";
  std::cout << "  --verbose, -v     Enable verbose logging\n\n";
  std::cout << "Examples:\n";
  std::cout << "  horcrux clean\n";
  std::cout << "  horcrux clean --cache\n";
  std::cout << "  horcrux clean --outputs --output-dir=./build\n";
  std::cout << "  horcrux clean --dry-run\n";
}

auto is_safe_path(const std::filesystem::path& path) -> bool {
  // Reject empty paths, root paths, and home directory root
  if (path.empty() || path == "/" || path == std::filesystem::path{"~"}) {
    return false;
  }

  // Reject absolute paths that are too short (e.g., /tmp, /usr, /home)
  if (path.is_absolute()) {
    int depth = 0;
    for ([[maybe_unused]] const auto& component : path) {
      ++depth;
    }
    if (depth < MIN_SAFE_ABSOLUTE_PATH_DEPTH) {
      return false;
    }
  }

  return true;
}

/// Remove a directory with proper error handling
auto remove_directory(const std::filesystem::path& dir, bool dry_run, bool verbose,
                      Logger& logger) -> tl::expected<void, CleanError> {
  if (!std::filesystem::exists(dir)) {
    if (verbose) {
      logger.info("  Skip (not found): ", dir.string());
    }
    return {};
  }

  if (!is_safe_path(dir)) {
    logger.error("Refusing to remove unsafe path: ", dir.string());
    return tl::unexpected(CleanError::InvalidPath);
  }

  if (dry_run) {
    std::cout << "  [dry-run] would remove: " << dir.string() << "\n";
    return {};
  }

  logger.info("  Removing: ", dir.string());
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  if (ec) {
    logger.error("Failed to remove ", dir.string(), ": ", ec.message());
    return tl::unexpected(CleanError::RemovalFailed);
  }
  return {};
}

auto handle_clean_command(int argc, char* argv[], Logger& logger) -> int {
  CleanOptions opts;
  bool scope_set = false;

  for (int i = 2; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--all") {
      opts.scope = CleanScope::All;
      scope_set = true;
    } else if (arg == "--outputs") {
      opts.scope = CleanScope::Outputs;
      scope_set = true;
    } else if (arg == "--cache") {
      opts.scope = CleanScope::Cache;
      scope_set = true;
    } else if (arg.starts_with("--output-dir=")) {
      opts.output_dir = arg.substr(13);
    } else if (arg.starts_with("--cache-dir=")) {
      opts.cache_dir = arg.substr(12);
    } else if (arg == "--dry-run") {
      opts.dry_run = true;
    } else if (arg == "--verbose" || arg == "-v") {
      opts.verbose = true;
    } else if (arg == "--help" || arg == "-h") {
      print_clean_usage();
      return 0;
    } else {
      logger.warning("Unknown option: ", arg);
    }
  }

  // Default scope is All when not explicitly set
  if (!scope_set) {
    opts.scope = CleanScope::All;
  }

  if (opts.verbose) {
    logger.set_level(LogLevel::Debug);
  }

  if (opts.dry_run) {
    std::cout << "[dry-run] The following paths would be removed:\n";
  }

  int exit_code = 0;

  // Remove build outputs
  if (opts.scope == CleanScope::All || opts.scope == CleanScope::Outputs) {
    logger.info("Cleaning build outputs...");
    auto result = remove_directory(opts.output_dir, opts.dry_run, opts.verbose, logger);
    if (!result) {
      exit_code = 1;
    }
  }

  // Remove cache
  if (opts.scope == CleanScope::All || opts.scope == CleanScope::Cache) {
    logger.info("Cleaning build cache...");
    auto result = remove_directory(opts.cache_dir, opts.dry_run, opts.verbose, logger);
    if (!result) {
      exit_code = 1;
    }
  }

  if (exit_code == 0) {
    if (opts.dry_run) {
      logger.info("Dry run complete. No files were removed.");
    } else {
      logger.info("Clean complete.");
    }
  }

  return exit_code;
}

} // namespace horcrux::cli
