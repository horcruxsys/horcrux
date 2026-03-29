// Horcrux - Test Command
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <tl/expected.hpp>

#include "logger.h"

namespace horcrux::cli {

/// @brief Options for the test command
struct TestOptions {
  std::vector<std::string> targets; ///< Target specs to test
  std::filesystem::path cache_dir = ".horcrux-cache";
  bool verbose = false;
};

/// @brief Result summary for a test run
struct TestResult {
  int total = 0;
  int passed = 0;
  int failed = 0;
  int skipped = 0;
};

/// @brief Error types for the test command
enum class TestError {
  NoTargetsSpecified,
  InvalidTarget,
  BuildFailed,
  ExecutionFailed,
};

/// @brief Convert TestError to human-readable string
[[nodiscard]] auto to_string(TestError error) -> std::string;

/// @brief Handle the `horcrux test` command
/// @param argc Argument count
/// @param argv Argument vector
/// @param logger Logger instance
/// @return Exit code (0 = all tests passed, 1 = failures or error)
auto handle_test_command(int argc, char* argv[], Logger& logger) -> int;

} // namespace horcrux::cli
