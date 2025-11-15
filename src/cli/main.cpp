// Horcrux - CLI Main Entry Point
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "build_executor.h"
#include "doctor_command.h"
#include "import_command.h"
#include "logger.h"

namespace horcrux::cli {

constexpr std::string_view VERSION = "0.1.0-alpha";

void print_version() {
  std::cout << "Horcrux Build System v" << VERSION << "\n";
  std::cout << "Built with C++23\n";
}

void print_usage() {
  std::cout << "Usage: horcrux [command] [options]\n\n";
  std::cout << "Commands:\n";
  std::cout << "  build <target>    Build the specified target (e.g., //examples/hello:app)\n";
  std::cout << "  import <path>     Import Gradle project and generate horcrux.yaml\n";
  std::cout << "  doctor <system>   Validate toolchain and system configuration\n";
  std::cout << "  test              Run tests (not yet implemented)\n";
  std::cout << "  clean             Remove build artifacts (not yet implemented)\n";
  std::cout << "  query             Query the build graph (not yet implemented)\n";
  std::cout << "  version           Show version information\n";
  std::cout << "  help              Show this help message\n\n";
  std::cout << "Options:\n";
  std::cout << "  --verbose, -v     Enable verbose logging\n";
  std::cout << "  --cache-dir=DIR   Set cache directory (default: .horcrux-cache)\n";
  std::cout << "  --output=FILE, -o Output file path for import command\n";
}

auto handle_build_command(int argc, char* argv[], Logger& logger) -> int {
  if (argc < 3) {
    logger.error("Missing target specification");
    std::cerr << "Usage: horcrux build <target>\n";
    std::cerr << "Example: horcrux build //examples/hello:app\n";
    return 1;
  }

  std::string target = argv[2];

  // Determine cache directory
  std::filesystem::path cache_dir = ".horcrux-cache";
  for (int i = 3; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg.starts_with("--cache-dir=")) {
      cache_dir = arg.substr(12);
    }
  }

  // Create build executor
  auto executor_result = BuildExecutor::create(cache_dir, logger);
  if (!executor_result) {
    logger.error("Failed to create build executor: ", to_string(executor_result.error()));
    return 1;
  }

  auto& executor = *executor_result;

  // Execute build
  logger.info("Starting build...");
  auto build_result = executor.build(target);
  if (!build_result) {
    logger.error("Build failed: ", to_string(build_result.error()));
    return 1;
  }

  logger.info("Build completed successfully!");
  return 0;
}

} // namespace horcrux::cli

int main(int argc, char* argv[]) {
  using namespace horcrux::cli;

  if (argc < 2) {
    print_usage();
    return 0;
  }

  std::string_view command = argv[1];

  // Check for verbose flag
  bool verbose = false;
  for (int i = 2; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--verbose" || arg == "-v") {
      verbose = true;
      break;
    }
  }

  // Set up logger
  if (verbose) {
    global_logger.set_level(LogLevel::Debug);
  }

  // Handle commands
  if (command == "version" || command == "--version" || command == "-v") {
    print_version();
    return 0;
  }

  if (command == "help" || command == "--help" || command == "-h") {
    print_usage();
    return 0;
  }

  if (command == "build") {
    return handle_build_command(argc, argv, global_logger);
  }

  if (command == "import") {
    return handle_import_command(argc, argv, global_logger);
  }

  if (command == "doctor") {
    return handle_doctor_command(argc, argv, global_logger);
  }

  // Unknown command
  std::cerr << "Unknown command: " << command << "\n";
  std::cerr << "Run 'horcrux help' for usage information.\n";
  return 1;
}
