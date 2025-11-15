// Horcrux - Android NDK C/C++ Compiler Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_ndk_compiler.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>

namespace horcrux::core {

namespace {

// Check if file has C extension
auto is_c_file(const std::filesystem::path& path) -> bool {
  auto ext = path.extension().string();
  return ext == ".c";
}

// Check if file has C++ extension
auto is_cpp_file(const std::filesystem::path& path) -> bool {
  auto ext = path.extension().string();
  return ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".C";
}

// Execute command and capture output
auto execute_command_impl(const std::vector<std::string>& command) -> std::pair<std::string, int> {
  // Build command string
  std::ostringstream cmd_stream;
  for (size_t i = 0; i < command.size(); ++i) {
    if (i > 0) {
      cmd_stream << " ";
    }
    // Simple quoting for arguments with spaces
    if (command[i].find(' ') != std::string::npos) {
      cmd_stream << "\"" << command[i] << "\"";
    } else {
      cmd_stream << command[i];
    }
  }
  cmd_stream << " 2>&1"; // Redirect stderr to stdout

  std::string cmd_str = cmd_stream.str();
  std::string output;

  // Execute command and capture output
#ifdef _WIN32
  FILE* pipe = _popen(cmd_str.c_str(), "r");
#else
  FILE* pipe = popen(cmd_str.c_str(), "r");
#endif

  if (pipe == nullptr) {
    return {"Failed to execute command", -1};
  }

  std::array<char, 128> buffer;
  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    output += buffer.data();
  }

#ifdef _WIN32
  int exit_code = _pclose(pipe);
#else
  int exit_code = pclose(pipe);
#endif

  return {output, exit_code};
}

} // anonymous namespace

auto to_string(AndroidAbi abi) -> std::string {
  switch (abi) {
  case AndroidAbi::Arm64V8a:
    return "arm64-v8a";
  case AndroidAbi::ArmeabiV7a:
    return "armeabi-v7a";
  case AndroidAbi::X86:
    return "x86";
  case AndroidAbi::X86_64:
    return "x86_64";
  }
  return "unknown";
}

auto abi_from_string(const std::string& str) -> std::optional<AndroidAbi> {
  if (str == "arm64-v8a") {
    return AndroidAbi::Arm64V8a;
  } else if (str == "armeabi-v7a") {
    return AndroidAbi::ArmeabiV7a;
  } else if (str == "x86") {
    return AndroidAbi::X86;
  } else if (str == "x86_64") {
    return AndroidAbi::X86_64;
  }
  return std::nullopt;
}

auto to_string(NdkCompilerError error) -> std::string {
  switch (error) {
  case NdkCompilerError::NdkNotFound:
    return "Android NDK not found";
  case NdkCompilerError::ToolchainNotFound:
    return "NDK toolchain not found";
  case NdkCompilerError::UnsupportedAbi:
    return "Unsupported ABI";
  case NdkCompilerError::CompilationFailed:
    return "Compilation failed";
  case NdkCompilerError::LinkingFailed:
    return "Linking failed";
  case NdkCompilerError::InvalidSourceFile:
    return "Invalid source file";
  case NdkCompilerError::InvalidOutputPath:
    return "Invalid output path";
  case NdkCompilerError::SysrootNotFound:
    return "Sysroot not found";
  case NdkCompilerError::ClangNotFound:
    return "Clang compiler not found";
  case NdkCompilerError::LldNotFound:
    return "LLD linker not found";
  case NdkCompilerError::IoError:
    return "I/O error";
  case NdkCompilerError::UnknownError:
    return "Unknown error";
  }
  return "Unknown error";
}

auto get_all_abis() -> std::vector<AndroidAbi> {
  return {AndroidAbi::Arm64V8a, AndroidAbi::ArmeabiV7a, AndroidAbi::X86, AndroidAbi::X86_64};
}

AndroidNdkCompiler::AndroidNdkCompiler(NdkAbiToolchain toolchain)
    : toolchain_(std::move(toolchain)) {
}

auto AndroidNdkCompiler::create(const AndroidNdk& ndk, AndroidAbi abi, const std::string& api_level)
    -> tl::expected<AndroidNdkCompiler, NdkCompilerError> {
  auto toolchain_result = detect_abi_toolchain(ndk, abi, api_level);
  if (!toolchain_result) {
    return tl::unexpected(toolchain_result.error());
  }

  return AndroidNdkCompiler(std::move(*toolchain_result));
}

auto AndroidNdkCompiler::detect_abi_toolchain(const AndroidNdk& ndk, AndroidAbi abi,
                                              const std::string& api_level)
    -> tl::expected<NdkAbiToolchain, NdkCompilerError> {
  NdkAbiToolchain toolchain;
  toolchain.abi = abi;
  toolchain.ndk_root = ndk.path;
  toolchain.api_level = api_level;
  toolchain.target_triple = get_target_triple(abi, api_level);

  // Detect toolchain root (NDK r19+ uses llvm directory)
  auto llvm_dir = ndk.path / "toolchains" / "llvm" / "prebuilt";
  if (!std::filesystem::exists(llvm_dir)) {
    return tl::unexpected(NdkCompilerError::ToolchainNotFound);
  }

  // Find the prebuilt host directory (linux-x86_64, darwin-x86_64, windows-x86_64, etc.)
  std::filesystem::path host_dir;
  for (const auto& entry : std::filesystem::directory_iterator(llvm_dir)) {
    if (entry.is_directory()) {
      host_dir = entry.path();
      break;
    }
  }

  if (host_dir.empty() || !std::filesystem::exists(host_dir)) {
    return tl::unexpected(NdkCompilerError::ToolchainNotFound);
  }

  toolchain.toolchain_root = host_dir;
  toolchain.sysroot = host_dir / "sysroot";

  // Check sysroot exists
  if (!std::filesystem::exists(toolchain.sysroot)) {
    return tl::unexpected(NdkCompilerError::SysrootNotFound);
  }

  // Detect compiler paths
#ifdef _WIN32
  const std::string exe_ext = ".exe";
  const std::string cmd_ext = ".cmd";
#else
  const std::string exe_ext = "";
  const std::string cmd_ext = "";
#endif

  auto bin_dir = host_dir / "bin";
  toolchain.clang_path = bin_dir / ("clang" + exe_ext);
  toolchain.clangxx_path = bin_dir / ("clang++" + exe_ext);
  toolchain.ar_path = bin_dir / ("llvm-ar" + exe_ext);
  toolchain.ld_path = bin_dir / ("ld.lld" + exe_ext);
  toolchain.strip_path = bin_dir / ("llvm-strip" + exe_ext);

  // Verify clang exists
  if (!std::filesystem::exists(toolchain.clang_path)) {
    return tl::unexpected(NdkCompilerError::ClangNotFound);
  }

  // Set up include paths
  auto arch_name = get_arch_name(abi);
  toolchain.include_paths = {
      (toolchain.sysroot / "usr" / "include").string(),
      (toolchain.sysroot / "usr" / "include" / toolchain.target_triple).string(),
      (host_dir / "include" / "c++" / "v1").string()};

  // Set up library paths
  toolchain.library_paths = {
      (toolchain.sysroot / "usr" / "lib" / toolchain.target_triple / api_level).string(),
      (toolchain.sysroot / "usr" / "lib" / toolchain.target_triple).string()};

  return toolchain;
}

auto AndroidNdkCompiler::get_target_triple(AndroidAbi abi,
                                           const std::string& api_level) -> std::string {
  switch (abi) {
  case AndroidAbi::Arm64V8a:
    return "aarch64-linux-android";
  case AndroidAbi::ArmeabiV7a:
    return "armv7a-linux-androideabi";
  case AndroidAbi::X86:
    return "i686-linux-android";
  case AndroidAbi::X86_64:
    return "x86_64-linux-android";
  }
  return "unknown-linux-android";
}

auto AndroidNdkCompiler::get_arch_name(AndroidAbi abi) -> std::string {
  switch (abi) {
  case AndroidAbi::Arm64V8a:
    return "aarch64";
  case AndroidAbi::ArmeabiV7a:
    return "arm";
  case AndroidAbi::X86:
    return "i686";
  case AndroidAbi::X86_64:
    return "x86_64";
  }
  return "unknown";
}

auto AndroidNdkCompiler::compile(
    const std::filesystem::path& source_file, const std::filesystem::path& output_file,
    const NdkCompileOptions& options) -> tl::expected<NdkCompileResult, NdkCompilerError> {
  // Validate source file
  if (!std::filesystem::exists(source_file)) {
    return tl::unexpected(NdkCompilerError::InvalidSourceFile);
  }

  // Build compile command
  auto command = build_compile_command(source_file, output_file, options);

  // Execute compilation
  auto result = execute_command(command);
  if (!result) {
    return tl::unexpected(result.error());
  }

  auto [output, exit_code] = *result;

  // Build result
  NdkCompileResult compile_result;
  compile_result.object_file = output_file;
  compile_result.output = output;
  compile_result.exit_code = exit_code;

  // Build command string for logging
  std::ostringstream cmd_stream;
  for (size_t i = 0; i < command.size(); ++i) {
    if (i > 0) {
      cmd_stream << " ";
    }
    cmd_stream << command[i];
  }
  compile_result.command = cmd_stream.str();

  if (exit_code != 0) {
    return tl::unexpected(NdkCompilerError::CompilationFailed);
  }

  return compile_result;
}

auto AndroidNdkCompiler::build_compile_command(
    const std::filesystem::path& source_file, const std::filesystem::path& output_file,
    const NdkCompileOptions& options) -> std::vector<std::string> {
  std::vector<std::string> command;

  // Choose compiler based on file type
  if (is_cpp_file(source_file)) {
    command.push_back(toolchain_.clangxx_path.string());
  } else {
    command.push_back(toolchain_.clang_path.string());
  }

  // Target and sysroot
  command.push_back("--target=" + toolchain_.target_triple + toolchain_.api_level);
  command.push_back("--sysroot=" + toolchain_.sysroot.string());

  // Optimization level
  command.push_back(options.optimization_level);

  // Debug info
  if (options.debug_info) {
    command.push_back("-g");
  }

  // Position independent code
  if (options.pic) {
    command.push_back("-fPIC");
  }

  // C++ specific flags
  if (is_cpp_file(source_file)) {
    command.push_back("-std=" + options.cpp_std);

    if (!options.exceptions) {
      command.push_back("-fno-exceptions");
    }

    if (!options.rtti) {
      command.push_back("-fno-rtti");
    }
  }

  // Include paths
  for (const auto& include : toolchain_.include_paths) {
    command.push_back("-isystem");
    command.push_back(include);
  }

  for (const auto& include : options.include_dirs) {
    command.push_back("-I" + include);
  }

  // Defines
  for (const auto& define : options.defines) {
    command.push_back("-D" + define);
  }

  // Additional flags
  if (is_cpp_file(source_file)) {
    for (const auto& flag : options.cxxflags) {
      command.push_back(flag);
    }
  } else {
    for (const auto& flag : options.cflags) {
      command.push_back(flag);
    }
  }

  // Compile only (don't link)
  command.push_back("-c");
  command.push_back(source_file.string());
  command.push_back("-o");
  command.push_back(output_file.string());

  return command;
}

auto AndroidNdkCompiler::link(const std::vector<std::filesystem::path>& object_files,
                              const std::filesystem::path& output_file,
                              const NdkLinkOptions& options)
    -> tl::expected<NdkLinkResult, NdkCompilerError> {
  // Validate object files
  for (const auto& obj : object_files) {
    if (!std::filesystem::exists(obj)) {
      return tl::unexpected(NdkCompilerError::InvalidSourceFile);
    }
  }

  // Build link command
  auto command = build_link_command(object_files, output_file, options);

  // Execute linking
  auto result = execute_command(command);
  if (!result) {
    return tl::unexpected(result.error());
  }

  auto [output, exit_code] = *result;

  // Build result
  NdkLinkResult link_result;
  link_result.output_file = output_file;
  link_result.output = output;
  link_result.exit_code = exit_code;

  // Build command string for logging
  std::ostringstream cmd_stream;
  for (size_t i = 0; i < command.size(); ++i) {
    if (i > 0) {
      cmd_stream << " ";
    }
    cmd_stream << command[i];
  }
  link_result.command = cmd_stream.str();

  if (exit_code != 0) {
    return tl::unexpected(NdkCompilerError::LinkingFailed);
  }

  // Strip symbols if requested
  if (options.strip_symbols && std::filesystem::exists(toolchain_.strip_path)) {
    std::vector<std::string> strip_cmd = {toolchain_.strip_path.string(), output_file.string()};
    execute_command(strip_cmd); // Ignore errors
  }

  return link_result;
}

auto AndroidNdkCompiler::build_link_command(const std::vector<std::filesystem::path>& object_files,
                                            const std::filesystem::path& output_file,
                                            const NdkLinkOptions& options)
    -> std::vector<std::string> {
  std::vector<std::string> command;

  // Use clang++ for linking (it handles C++ standard library automatically)
  command.push_back(toolchain_.clangxx_path.string());

  // Target and sysroot
  command.push_back("--target=" + toolchain_.target_triple + toolchain_.api_level);
  command.push_back("--sysroot=" + toolchain_.sysroot.string());

  // Shared library flag
  if (options.shared) {
    command.push_back("-shared");
  }

  // Object files
  for (const auto& obj : object_files) {
    command.push_back(obj.string());
  }

  // Library paths
  for (const auto& lib_path : toolchain_.library_paths) {
    command.push_back("-L" + lib_path);
  }

  for (const auto& lib_path : options.library_dirs) {
    command.push_back("-L" + lib_path);
  }

  // Libraries
  for (const auto& lib : options.libraries) {
    command.push_back("-l" + lib);
  }

  // Additional linker flags
  for (const auto& flag : options.ldflags) {
    command.push_back(flag);
  }

  // Output file
  command.push_back("-o");
  command.push_back(output_file.string());

  return command;
}

auto AndroidNdkCompiler::compile_and_link(
    const std::vector<std::filesystem::path>& source_files,
    const std::filesystem::path& output_file, const NdkCompileOptions& compile_opts,
    const NdkLinkOptions& link_opts) -> tl::expected<NdkLinkResult, NdkCompilerError> {
  // Create temporary directory for object files
  auto temp_dir = std::filesystem::temp_directory_path() / "horcrux_ndk_build";
  std::filesystem::create_directories(temp_dir);

  std::vector<std::filesystem::path> object_files;

  // Compile all source files
  for (const auto& source : source_files) {
    auto obj_name = source.stem().string() + ".o";
    auto obj_path = temp_dir / obj_name;

    auto compile_result = compile(source, obj_path, compile_opts);
    if (!compile_result) {
      return tl::unexpected(compile_result.error());
    }

    object_files.push_back(obj_path);
  }

  // Link all object files
  auto link_result = link(object_files, output_file, link_opts);

  // Clean up temporary object files
  std::error_code ec;
  std::filesystem::remove_all(temp_dir, ec);

  return link_result;
}

auto AndroidNdkCompiler::execute_command(const std::vector<std::string>& command)
    -> tl::expected<std::pair<std::string, int>, NdkCompilerError> {
  auto [output, exit_code] = execute_command_impl(command);
  return std::make_pair(output, exit_code);
}

auto AndroidNdkCompiler::generate_cmake_toolchain_file(const std::filesystem::path& output_path)
    -> tl::expected<void, NdkCompilerError> {
  std::ofstream file(output_path);
  if (!file.is_open()) {
    return tl::unexpected(NdkCompilerError::InvalidOutputPath);
  }

  file << "# CMake Toolchain File for Android NDK\n";
  file << "# Generated by Horcrux\n";
  file << "# ABI: " << to_string(toolchain_.abi) << "\n";
  file << "# API Level: " << toolchain_.api_level << "\n\n";

  file << "set(CMAKE_SYSTEM_NAME Android)\n";
  file << "set(CMAKE_SYSTEM_VERSION " << toolchain_.api_level << ")\n";
  file << "set(CMAKE_ANDROID_ARCH_ABI " << to_string(toolchain_.abi) << ")\n";
  file << "set(CMAKE_ANDROID_NDK \"" << toolchain_.ndk_root.string() << "\")\n\n";

  file << "set(CMAKE_C_COMPILER \"" << toolchain_.clang_path.string() << "\")\n";
  file << "set(CMAKE_CXX_COMPILER \"" << toolchain_.clangxx_path.string() << "\")\n";
  file << "set(CMAKE_AR \"" << toolchain_.ar_path.string() << "\")\n";
  file << "set(CMAKE_RANLIB \"" << toolchain_.ar_path.string() << "\" CACHE FILEPATH \"Ranlib\")\n";
  file << "set(CMAKE_STRIP \"" << toolchain_.strip_path.string() << "\")\n\n";

  file << "set(CMAKE_SYSROOT \"" << toolchain_.sysroot.string() << "\")\n\n";

  file << "set(CMAKE_C_FLAGS_INIT \"--target=" << toolchain_.target_triple << toolchain_.api_level
       << "\")\n";
  file << "set(CMAKE_CXX_FLAGS_INIT \"--target=" << toolchain_.target_triple << toolchain_.api_level
       << "\")\n\n";

  file << "set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)\n";
  file << "set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)\n";
  file << "set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)\n";
  file << "set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)\n";

  file.close();
  return {};
}

auto detect_all_ndk_toolchains(const AndroidNdk& ndk, const std::string& api_level)
    -> tl::expected<std::vector<AndroidNdkCompiler>, NdkCompilerError> {
  std::vector<AndroidNdkCompiler> compilers;

  for (auto abi : get_all_abis()) {
    auto compiler_result = AndroidNdkCompiler::create(ndk, abi, api_level);
    if (compiler_result) {
      compilers.push_back(std::move(*compiler_result));
    }
  }

  if (compilers.empty()) {
    return tl::unexpected(NdkCompilerError::ToolchainNotFound);
  }

  return compilers;
}

} // namespace horcrux::core
