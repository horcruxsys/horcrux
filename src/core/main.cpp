// Horcrux - The Next-Generation Universal Build System
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <iostream>
#include <string_view>

#include "simple_builder.h"

namespace horcrux {

constexpr std::string_view VERSION = "0.1.0-bootstrap";

void print_version() {
  std::cout << "Horcrux Build System v" << VERSION << "\n";
  std::cout << "Built with C++23\n";
}

void print_usage() {
  std::cout << "Usage: horcrux [command] [options]\n\n";
  std::cout << "Commands:\n";
  std::cout << "  build     Build the specified targets\n";
  std::cout << "  test      Run tests\n";
  std::cout << "  run       Build and run a target\n";
  std::cout << "  clean     Remove build artifacts\n";
  std::cout << "  query     Query the build graph\n";
  std::cout << "  info      Show system information\n";
  std::cout << "  version   Show version information\n";
  std::cout << "  help      Show this help message\n";
}

} // namespace horcrux

int main(int argc, char* argv[]) {
  using namespace horcrux;

  if (argc < 2) {
    print_usage();
    return 0;
  }

  std::string_view command = argv[1];

  if (command == "version" || command == "--version" || command == "-v") {
    print_version();
    return 0;
  }

  if (command == "help" || command == "--help" || command == "-h") {
    print_usage();
    return 0;
  }

  // Handle build command
  if (command == "build") {
    if (argc < 3) {
      std::cerr << "Error: build command requires a target\n";
      std::cerr << "Usage: horcrux build //package/path:target_name\n";
      return 1;
    }

    core::SimpleBuilder builder;
    auto result = builder.build(argv[2]);

    if (!result) {
      std::cerr << "Build failed: " << core::to_string(result.error()) << "\n";
      return 1;
    }

    return 0;
  }

  // Handle clean command
  if (command == "clean") {
    core::SimpleBuilder builder;
    auto result = builder.clean();

    if (!result) {
      std::cerr << "Clean failed: " << core::to_string(result.error()) << "\n";
      return 1;
    }

    return 0;
  }

  // Other commands not yet implemented
  std::cout << "Horcrux v" << VERSION << " (bootstrap)\n";
  std::cout << "Command '" << command << "' is not yet implemented.\n";
  std::cout << "This is a minimal bootstrap build to set up the build system.\n";

  return 0;
}
