// Horcrux - Simple Builder Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "simple_builder.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>

namespace fs = std::filesystem;

namespace horcrux::core {

auto to_string(BuildError error) -> std::string {
  switch (error) {
  case BuildError::InvalidTarget:
    return "Invalid target format";
  case BuildError::CompilationFailed:
    return "Compilation failed";
  case BuildError::SourceNotFound:
    return "Source file not found";
  case BuildError::BuildFileNotFound:
    return "BUILD file not found";
  }
  return "Unknown error";
}

auto SimpleBuilder::parse_target(std::string_view target) -> tl::expected<TargetInfo, BuildError> {
  // Target format: //package/path:target_name
  std::regex target_regex(R"(^//([^:]+):([^:]+)$)");
  std::smatch matches;

  std::string target_str(target);
  if (!std::regex_match(target_str, matches, target_regex)) {
    return tl::unexpected(BuildError::InvalidTarget);
  }

  TargetInfo info;
  info.package_path = matches[1].str();
  info.target_name = matches[2].str();

  return info;
}

auto SimpleBuilder::build_file_exists(std::string_view package_path) -> bool {
  fs::path build_file_path = fs::path(package_path) / "BUILD";
  return fs::exists(build_file_path);
}

auto SimpleBuilder::compile_cc_binary(const TargetInfo& target_info)
    -> tl::expected<void, BuildError> {
  // Get source directory
  fs::path source_dir = fs::path(target_info.package_path);

  // Check if BUILD file exists
  if (!build_file_exists(target_info.package_path)) {
    std::cerr << "Error: BUILD file not found in " << target_info.package_path << "\n";
    return tl::unexpected(BuildError::BuildFileNotFound);
  }

  // For this bootstrap implementation, we assume main.cpp exists
  fs::path source_file = source_dir / "main.cpp";
  if (!fs::exists(source_file)) {
    std::cerr << "Error: Source file not found: " << source_file << "\n";
    return tl::unexpected(BuildError::SourceNotFound);
  }

  // Create output directory structure
  fs::path output_dir = fs::path("bazel-bin") / target_info.package_path;
  fs::create_directories(output_dir);

  fs::path output_binary = output_dir / target_info.target_name;

  // Determine compiler (prefer g++ or clang++)
  std::string compiler = "g++";
  if (std::system("which g++ > /dev/null 2>&1") != 0) {
    if (std::system("which clang++ > /dev/null 2>&1") == 0) {
      compiler = "clang++";
    } else {
      compiler = "c++"; // Fallback to generic c++
    }
  }

  // Build compilation command
  std::string compile_cmd = compiler + " -std=c++23 -O2 -Wall -Wextra " + source_file.string() +
                            " -o " + output_binary.string() + " 2>&1";

  std::cout << "Building target: //" << target_info.package_path << ":" << target_info.target_name
            << "\n";
  std::cout << "Compiling: " << source_file << "\n";
  std::cout << "Output: " << output_binary << "\n";

  // Execute compilation
  int result = std::system(compile_cmd.c_str());

  if (result != 0) {
    std::cerr << "Error: Compilation failed with exit code " << result << "\n";
    return tl::unexpected(BuildError::CompilationFailed);
  }

  std::cout << "✓ Build successful: " << output_binary << "\n";

  return {};
}

auto SimpleBuilder::build(std::string_view target) -> tl::expected<void, BuildError> {
  std::cout << "Horcrux Build System (Bootstrap)\n";
  std::cout << "=================================\n\n";

  // Parse target
  auto target_info = parse_target(target);
  if (!target_info) {
    std::cerr << "Error: Invalid target format '" << target << "'\n";
    std::cerr << "Expected format: //package/path:target_name\n";
    return tl::unexpected(target_info.error());
  }

  std::cout << "Target: " << target << "\n";
  std::cout << "Package: " << target_info->package_path << "\n";
  std::cout << "Name: " << target_info->target_name << "\n\n";

  // For this bootstrap, we only support cc_binary
  // A full implementation would parse the BUILD file and handle different rule types
  return compile_cc_binary(*target_info);
}

auto SimpleBuilder::clean() -> tl::expected<void, BuildError> {
  std::cout << "Cleaning build artifacts...\n";

  // Remove bazel-bin directory
  fs::path output_dir = "bazel-bin";
  if (fs::exists(output_dir)) {
    std::error_code ec;
    fs::remove_all(output_dir, ec);
    if (ec) {
      std::cerr << "Warning: Could not fully clean " << output_dir << ": " << ec.message() << "\n";
    } else {
      std::cout << "✓ Removed: " << output_dir << "\n";
    }
  }

  std::cout << "Clean complete.\n";
  return {};
}

} // namespace horcrux::core
