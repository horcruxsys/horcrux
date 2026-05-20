// Horcrux - Android NDK C/C++ Compiler for JNI
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "android_toolchain.h"

namespace horcrux::core {

// Android ABI (Application Binary Interface) types
enum class AndroidAbi {
  Arm64V8a,   // 64-bit ARM
  ArmeabiV7a, // 32-bit ARM
  X86,        // 32-bit x86
  X86_64      // 64-bit x86
};

// Convert ABI enum to string representation
auto to_string(AndroidAbi abi) -> std::string;

// Convert string to ABI enum
auto abi_from_string(const std::string& str) -> std::optional<AndroidAbi>;

// Error types for NDK compilation
enum class NdkCompilerError {
  NdkNotFound,
  ToolchainNotFound,
  UnsupportedAbi,
  CompilationFailed,
  LinkingFailed,
  InvalidSourceFile,
  InvalidOutputPath,
  SysrootNotFound,
  ClangNotFound,
  LldNotFound,
  IoError,
  UnknownError
};

// Convert error to string
auto to_string(NdkCompilerError error) -> std::string;

// NDK toolchain information for a specific ABI
struct NdkAbiToolchain {
  AndroidAbi abi;
  std::filesystem::path ndk_root;
  std::filesystem::path toolchain_root;
  std::filesystem::path sysroot;
  std::filesystem::path clang_path;
  std::filesystem::path clangxx_path;
  std::filesystem::path ar_path;
  std::filesystem::path ld_path;
  std::filesystem::path strip_path;
  std::string target_triple; // e.g., "aarch64-linux-android"
  std::string api_level;
  std::vector<std::string> include_paths;
  std::vector<std::string> library_paths;
};

// Compilation options for NDK
struct NdkCompileOptions {
  std::vector<std::string> defines;       // -D flags
  std::vector<std::string> include_dirs;  // -I flags
  std::vector<std::string> cflags;        // Additional C flags
  std::vector<std::string> cxxflags;      // Additional C++ flags
  std::string optimization_level = "-O2"; // -O0, -O1, -O2, -O3, -Os
  bool debug_info = false;                // -g flag
  bool pic = true;                        // Position independent code (-fPIC)
  bool exceptions = true;                 // C++ exceptions
  bool rtti = true;                       // C++ RTTI
  std::string cpp_std = "c++17";          // C++ standard version
};

// Linking options for NDK
struct NdkLinkOptions {
  std::vector<std::string> library_dirs; // -L flags
  std::vector<std::string> libraries;    // -l flags
  std::vector<std::string> ldflags;      // Additional linker flags
  bool strip_symbols = false;            // Strip debug symbols
  bool shared = true;                    // Build shared library (.so)
};

// Compilation result
struct NdkCompileResult {
  std::filesystem::path object_file;
  std::string command;
  std::string output;
  int exit_code;
};

// Linking result
struct NdkLinkResult {
  std::filesystem::path output_file;
  std::string command;
  std::string output;
  int exit_code;
};

// Android NDK Compiler
class AndroidNdkCompiler {
public:
  // Create compiler for specific ABI
  static auto create(const AndroidNdk& ndk, AndroidAbi abi, const std::string& api_level = "21")
      -> tl::expected<AndroidNdkCompiler, NdkCompilerError>;

  // Compile a single source file to object file
  auto compile(const std::filesystem::path& source_file, const std::filesystem::path& output_file,
               const NdkCompileOptions& options = {})
      -> tl::expected<NdkCompileResult, NdkCompilerError>;

  // Link object files into shared library
  auto link(const std::vector<std::filesystem::path>& object_files,
            const std::filesystem::path& output_file,
            const NdkLinkOptions& options = {}) -> tl::expected<NdkLinkResult, NdkCompilerError>;

  // Compile and link in one step
  auto compile_and_link(
      const std::vector<std::filesystem::path>& source_files,
      const std::filesystem::path& output_file, const NdkCompileOptions& compile_opts = {},
      const NdkLinkOptions& link_opts = {}) -> tl::expected<NdkLinkResult, NdkCompilerError>;

  // Get toolchain information
  auto get_toolchain() const -> const NdkAbiToolchain& {
    return toolchain_;
  }

  // Get ABI
  auto get_abi() const -> AndroidAbi {
    return toolchain_.abi;
  }

  // Generate CMake toolchain file for this ABI
  auto generate_cmake_toolchain_file(const std::filesystem::path& output_path)
      -> tl::expected<void, NdkCompilerError>;

private:
  explicit AndroidNdkCompiler(NdkAbiToolchain toolchain);

  // Detect toolchain for specific ABI
  static auto detect_abi_toolchain(const AndroidNdk& ndk, AndroidAbi abi,
                                   const std::string& api_level)
      -> tl::expected<NdkAbiToolchain, NdkCompilerError>;

  // Get target triple for ABI
  static auto get_target_triple(AndroidAbi abi, const std::string& api_level) -> std::string;

  // Get architecture name for ABI
  static auto get_arch_name(AndroidAbi abi) -> std::string;

  // Build compile command
  auto build_compile_command(const std::filesystem::path& source_file,
                             const std::filesystem::path& output_file,
                             const NdkCompileOptions& options) -> std::vector<std::string>;

  // Build link command
  auto build_link_command(const std::vector<std::filesystem::path>& object_files,
                          const std::filesystem::path& output_file,
                          const NdkLinkOptions& options) -> std::vector<std::string>;

  // Execute command and capture output
  static auto execute_command(const std::vector<std::string>& command)
      -> tl::expected<std::pair<std::string, int>, NdkCompilerError>;

  NdkAbiToolchain toolchain_;
};

// Helper function to get all supported ABIs
auto get_all_abis() -> std::vector<AndroidAbi>;

// Helper function to detect all available NDK toolchains
auto detect_all_ndk_toolchains(const AndroidNdk& ndk, const std::string& api_level = "21")
    -> tl::expected<std::vector<AndroidNdkCompiler>, NdkCompilerError>;

} // namespace horcrux::core
