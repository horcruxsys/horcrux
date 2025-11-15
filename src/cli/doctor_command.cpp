// Horcrux - Doctor Command Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "doctor_command.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

#include "../core/android_toolchain.h"

namespace horcrux::cli {

namespace {

void print_doctor_usage() {
  std::cout << "Usage: horcrux doctor [subsystem]\n\n";
  std::cout << "Subsystems:\n";
  std::cout << "  android     Validate Android SDK, NDK, and Java toolchain\n";
  std::cout << "  system      Check system requirements (not yet implemented)\n";
  std::cout << "  cache       Validate cache configuration (not yet implemented)\n\n";
  std::cout << "Examples:\n";
  std::cout << "  horcrux doctor android    # Check Android toolchain setup\n";
}

void print_check_status(const std::string& name, bool passed) {
  std::cout << "  [" << (passed ? "✓" : "✗") << "] " << name;
  if (passed) {
    std::cout << " (PASS)\n";
  } else {
    std::cout << " (FAIL)\n";
  }
}

void print_info(const std::string& label, const std::string& value) {
  std::cout << "    " << label << ": " << value << "\n";
}

} // anonymous namespace

auto handle_doctor_command(int argc, char* argv[], Logger& logger) -> int {
  if (argc < 3) {
    print_doctor_usage();
    return 0;
  }

  std::string_view subsystem = argv[2];

  if (subsystem == "android") {
    return handle_doctor_android(logger);
  }

  if (subsystem == "system" || subsystem == "cache") {
    logger.error("Subsystem '", subsystem, "' is not yet implemented");
    return 1;
  }

  logger.error("Unknown subsystem: ", subsystem);
  print_doctor_usage();
  return 1;
}

auto handle_doctor_android(Logger& logger) -> int {
  logger.info("Checking Android toolchain...");
  std::cout << "\n";

  // Detect Android toolchain
  auto result = core::AndroidToolchainDetector::detect();

  if (!result) {
    std::cout << "❌ Android Toolchain Detection Failed\n\n";
    std::cout << "Error: " << core::to_string(result.error()) << "\n\n";

    // Provide helpful hints
    std::cout << "Please ensure:\n";
    std::cout << "  1. Android SDK is installed\n";
    std::cout << "  2. ANDROID_HOME or ANDROID_SDK_ROOT environment variable is set\n";
    std::cout << "  3. Required SDK components are installed:\n";
    std::cout << "     - build-tools\n";
    std::cout << "     - platforms\n";
    std::cout << "     - platform-tools (recommended)\n";
    std::cout << "\n";

    return 1;
  }

  const auto& toolchain = *result;

  std::cout << "✓ Android Toolchain Detected\n\n";

  // SDK Information
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "Android SDK\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  print_info("SDK Root", toolchain.sdk_root.string());

  if (toolchain.platform_tools) {
    print_info("Platform Tools", toolchain.platform_tools->string());
  }

  if (toolchain.cmdline_tools) {
    print_info("Command Line Tools", toolchain.cmdline_tools->string());
  }

  std::cout << "\n";

  // Build Tools
  if (!toolchain.build_tools.empty()) {
    std::cout << "Build Tools (" << toolchain.build_tools.size() << " version(s) found):\n";
    for (const auto& bt : toolchain.build_tools) {
      print_info("Version " + bt.version, bt.path.string());
    }
    std::cout << "\n";
  } else {
    std::cout << "⚠ No build tools found\n\n";
  }

  // Platforms
  if (!toolchain.platforms.empty()) {
    std::cout << "Platforms (" << toolchain.platforms.size() << " API level(s) found):\n";
    for (const auto& platform : toolchain.platforms) {
      print_info("API Level " + platform.api_level, platform.path.string());
    }
    std::cout << "\n";
  } else {
    std::cout << "⚠ No platforms found\n\n";
  }

  // NDK Information
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "Android NDK\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

  if (toolchain.ndk_root) {
    print_info("NDK Root", toolchain.ndk_root->string());

    if (!toolchain.ndks.empty()) {
      std::cout << "\nNDK Versions (" << toolchain.ndks.size() << " version(s) found):\n";
      for (const auto& ndk : toolchain.ndks) {
        print_info("Version " + ndk.version, ndk.path.string());
      }
    } else {
      std::cout << "⚠ No NDK versions detected\n";
    }
  } else {
    std::cout << "⚠ NDK not found (optional)\n";
    std::cout << "  Set ANDROID_NDK_ROOT or install NDK via SDK Manager\n";
  }

  std::cout << "\n";

  // Java Information
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "Java SDK\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

  if (toolchain.java_sdk) {
    print_info("Java Version", toolchain.java_sdk->version);
    print_info("JAVA_HOME", toolchain.java_sdk->java_home.string());
    print_info("javac", toolchain.java_sdk->javac_path.string());
    print_info("java", toolchain.java_sdk->java_path.string());
  } else {
    std::cout << "⚠ Java SDK not found (optional for Java/Kotlin builds)\n";
    std::cout << "  Set JAVA_HOME to enable Java/Kotlin compilation\n";
  }

  std::cout << "\n";

  // Reproducibility Information
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "Reproducibility\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  print_info("Toolchain Hash", toolchain.merkle_hash);
  std::cout << "\n";

  // Validation
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "Validation\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

  auto validation_result = core::AndroidToolchainDetector::validate(toolchain);
  if (validation_result) {
    print_check_status("Toolchain configuration valid", true);
  } else {
    print_check_status("Toolchain configuration valid", false);
    std::cout << "    Error: " << core::to_string(validation_result.error()) << "\n";
  }

  print_check_status("SDK structure valid", 
                    core::AndroidToolchainValidator::validate_sdk(toolchain.sdk_root));

  if (toolchain.ndk_root) {
    print_check_status("NDK structure valid",
                      core::AndroidToolchainValidator::validate_ndk(*toolchain.ndk_root));
  }

  if (toolchain.java_sdk) {
    print_check_status("Java installation valid",
                      core::AndroidToolchainValidator::validate_java_home(
                          toolchain.java_sdk->java_home));
  }

  std::cout << "\n";

  // Save toolchain manifest
  try {
    std::filesystem::path lock_dir = ".horcrux/lock";
    std::filesystem::create_directories(lock_dir);

    std::filesystem::path manifest_path = lock_dir / "android-toolchain.json";
    std::ofstream manifest_file(manifest_path);
    if (manifest_file.is_open()) {
      manifest_file << toolchain.to_json();
      manifest_file.close();
      
      std::cout << "✓ Toolchain manifest saved to: " << manifest_path.string() << "\n";
      std::cout << "  This file ensures reproducible builds across machines.\n";
    } else {
      std::cout << "⚠ Failed to save toolchain manifest\n";
    }
  } catch (const std::exception& e) {
    std::cout << "⚠ Failed to save toolchain manifest: " << e.what() << "\n";
  }

  std::cout << "\n";

  // Summary
  bool has_warnings = !toolchain.ndk_root || !toolchain.java_sdk || 
                     toolchain.build_tools.empty() || toolchain.platforms.empty();

  if (has_warnings) {
    std::cout << "✓ Android toolchain detected with warnings\n";
    std::cout << "  Review the warnings above to ensure all required components are installed.\n";
  } else {
    std::cout << "✓ Android toolchain fully configured\n";
    std::cout << "  All components detected successfully!\n";
  }

  std::cout << "\n";

  return 0;
}

} // namespace horcrux::cli
