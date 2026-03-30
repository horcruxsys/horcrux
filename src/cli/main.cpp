// Horcrux - CLI Main Entry Point
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "build_executor.h"
#include "clean_command.h"
#include "doctor_command.h"
#include "import_command.h"
#include "logger.h"
#include "query_command.h"
#include "test_command.h"
#include "../core/sandbox_policy.h"

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
  std::cout << "  test <target>...  Execute tests for the specified target(s)\n";
  std::cout << "  clean             Remove build outputs and/or local cache artifacts\n";
  std::cout << "  query <target>    Query build graph metadata (deps, rdeps, topo order)\n";
  std::cout << "  import <path>     Import Gradle project and generate horcrux.yaml\n";
  std::cout << "  doctor <system>   Validate toolchain and system configuration\n";
  std::cout << "  version           Show version information\n";
  std::cout << "  help              Show this help message\n\n";
  std::cout << "Options:\n";
  std::cout << "  --verbose, -v          Enable verbose logging\n";
  std::cout << "  --cache-dir=DIR        Set cache directory (default: .horcrux-cache)\n";
  std::cout << "  --output=FILE, -o      Output file path for import command\n";
  std::cout << "  --hermetic             Enable hermetic (balanced) sandboxing (default)\n";
  std::cout << "  --sandbox=MODE         Sandbox mode: strict | balanced | off\n";
  std::cout << "  --repro-check          Run double-build reproducibility check\n\n";
  std::cout << "Run 'horcrux <command> --help' for detailed usage of each command.\n";
}

auto handle_build_command(int argc, char* argv[], Logger& logger) -> int {
  if (argc < 3) {
    logger.error("Missing target specification");
    std::cerr << "Usage: horcrux build <target>\n";
    std::cerr << "Example: horcrux build //examples/hello:app\n";
    return 1;
  }

  std::string target = argv[2];

  // Determine cache directory and sandbox options
  std::filesystem::path cache_dir = ".horcrux-cache";
  bool hermetic_flag = false;
  bool repro_check = false;
  std::string sandbox_mode_str;

  for (int i = 3; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg.starts_with("--cache-dir=")) {
      cache_dir = arg.substr(12);
    } else if (arg == "--hermetic") {
      hermetic_flag = true;
    } else if (arg.starts_with("--sandbox=")) {
      sandbox_mode_str = std::string(arg.substr(10));
    } else if (arg == "--repro-check") {
      repro_check = true;
    }
  }

  // Resolve sandbox policy
  core::SandboxPolicy policy = core::SandboxPolicy::default_hermetic();
  if (!sandbox_mode_str.empty()) {
    auto mode_result = core::sandbox_mode_from_string(sandbox_mode_str);
    if (!mode_result) {
      logger.error("Invalid --sandbox value: ", sandbox_mode_str,
                   " (expected: strict | balanced | off)");
      return 1;
    }
    switch (*mode_result) {
    case core::SandboxMode::Off:
      policy = core::SandboxPolicy::off();
      break;
    case core::SandboxMode::Strict:
      policy = core::SandboxPolicy::strict_hermetic();
      break;
    case core::SandboxMode::Balanced:
      policy = core::SandboxPolicy::default_hermetic();
      break;
    }
  } else if (hermetic_flag) {
    policy = core::SandboxPolicy::default_hermetic();
  }

  // Create build executor with policy
  auto executor_result = BuildExecutor::create_with_policy(cache_dir, policy, repro_check, logger);
  if (!executor_result) {
    logger.error("Failed to create build executor: ", to_string(executor_result.error()));
    return 1;
  }

  auto& executor = *executor_result;

  // Execute build
  logger.info("Starting build...");
  if (policy.mode != core::SandboxMode::Off) {
    logger.info("Sandbox: ", core::to_string(policy.mode));
  }
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

  if (command == "test") {
    return handle_test_command(argc, argv, global_logger);
  }

  if (command == "clean") {
    return handle_clean_command(argc, argv, global_logger);
  }

  if (command == "query") {
    return handle_query_command(argc, argv, global_logger);
  }

  // Unknown command
  std::cerr << "Unknown command: " << command << "\n";
  std::cerr << "Run 'horcrux help' for usage information.\n";
  return 1;
}
