// Horcrux - Android D8/R8 DEX Compiler Rules
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

/// @brief Error types for DEX compilation
/// Used with tl::expected for recoverable error handling
enum class DexCompilerError {
  CompilerNotFound,
  InvalidInputFile,
  InvalidConfiguration,
  CompilationFailed,
  ProguardConfigNotFound,
  MergeFailed,
  IoError,
  UnknownError
};

// Convert error to string
auto to_string(DexCompilerError error) -> std::string;

// ProGuard configuration
struct ProguardConfig {
  std::vector<std::filesystem::path> config_files;
  std::vector<std::string> keep_rules;
  std::vector<std::string> keep_attributes;
  bool optimize = true;
  bool obfuscate = true;
  bool shrink = true;
  std::optional<std::filesystem::path> mapping_output;
  std::optional<std::filesystem::path> usage_output;
  std::optional<std::filesystem::path> seeds_output;
};

// D8 compilation configuration
struct D8CompileConfig {
  // Input .class files or JAR files
  std::vector<std::filesystem::path> inputs;

  // Classpath for resolving references
  std::vector<std::filesystem::path> classpath;

  // Output directory for .dex files
  std::filesystem::path output_dir;

  // Minimum Android API level
  int min_api = 21;

  // Enable multi-dex support
  bool multi_dex = false;

  // Main dex list file (for multi-dex)
  std::optional<std::filesystem::path> main_dex_list;

  // Enable debug info in DEX
  bool debug = false;

  // Enable incremental compilation
  bool incremental = true;

  // Additional D8 options
  std::vector<std::string> d8_options;

  // Enable verbose output
  bool verbose = false;
};

// R8 compilation configuration (includes D8 + optimization)
struct R8CompileConfig {
  // Input .class files or JAR files
  std::vector<std::filesystem::path> inputs;

  // Classpath for resolving references
  std::vector<std::filesystem::path> classpath;

  // Output directory for .dex files
  std::filesystem::path output_dir;

  // Minimum Android API level
  int min_api = 21;

  // ProGuard configuration
  ProguardConfig proguard_config;

  // Enable multi-dex support
  bool multi_dex = false;

  // Main dex list file (for multi-dex)
  std::optional<std::filesystem::path> main_dex_list;

  // Enable debug info in DEX
  bool debug = false;

  // Enable incremental compilation
  bool incremental = true;

  // Additional R8 options
  std::vector<std::string> r8_options;

  // Enable verbose output
  bool verbose = false;
};

// DEX merge configuration
struct DexMergeConfig {
  // Input .dex files to merge
  std::vector<std::filesystem::path> dex_files;

  // Output .dex file
  std::filesystem::path output_file;

  // Minimum Android API level
  int min_api = 21;

  // Enable verbose output
  bool verbose = false;
};

// DEX compilation result
struct DexCompileResult {
  // Success status
  bool success;

  // Output directory containing .dex files
  std::filesystem::path output_dir;

  // List of generated .dex files (e.g., classes.dex, classes2.dex)
  std::vector<std::filesystem::path> dex_files;

  // Compilation hash for caching
  std::string compilation_hash;

  // Compilation time in milliseconds
  std::chrono::milliseconds compilation_time;

  // Standard output from compiler
  std::string stdout_output;

  // Standard error from compiler
  std::string stderr_output;

  // R8-specific: mapping file path (if R8 was used)
  std::optional<std::filesystem::path> mapping_file;

  // R8-specific: shrunk classes count
  std::optional<size_t> shrunk_classes_count;
};

// Android DEX compiler rule (D8/R8)
class AndroidDexCompiler {
public:
  // Create compiler with toolchain
  explicit AndroidDexCompiler(const AndroidToolchain& toolchain);

  // Compile with D8 (Java bytecode -> DEX)
  auto
  compile_d8(const D8CompileConfig& config) -> tl::expected<DexCompileResult, DexCompilerError>;

  // Compile with R8 (Java bytecode -> optimized DEX)
  auto
  compile_r8(const R8CompileConfig& config) -> tl::expected<DexCompileResult, DexCompilerError>;

  // Merge multiple DEX files
  auto merge_dex(const DexMergeConfig& config) -> tl::expected<DexCompileResult, DexCompilerError>;

  // Validate D8 configuration
  static auto
  validate_d8_config(const D8CompileConfig& config) -> tl::expected<void, DexCompilerError>;

  // Validate R8 configuration
  static auto
  validate_r8_config(const R8CompileConfig& config) -> tl::expected<void, DexCompilerError>;

  // Compute compilation hash for caching (Merkle signature)
  static auto compute_d8_hash(const D8CompileConfig& config) -> std::string;
  static auto compute_r8_hash(const R8CompileConfig& config) -> std::string;

  // Check if compilation is needed (incremental compilation)
  auto is_d8_compilation_needed(const D8CompileConfig& config,
                                const std::string& cached_hash) const -> bool;
  auto is_r8_compilation_needed(const R8CompileConfig& config,
                                const std::string& cached_hash) const -> bool;

  // Parse ProGuard configuration file
  static auto parse_proguard_config(const std::filesystem::path& config_path)
      -> tl::expected<ProguardConfig, DexCompilerError>;

private:
  const AndroidToolchain& toolchain_;

  // Build D8 command line
  auto build_d8_command(const D8CompileConfig& config) const -> std::vector<std::string>;

  // Build R8 command line
  auto build_r8_command(const R8CompileConfig& config) const -> std::vector<std::string>;

  // Execute D8 command
  auto execute_d8(const std::vector<std::string>& command) const
      -> tl::expected<DexCompileResult, DexCompilerError>;

  // Execute R8 command
  auto execute_r8(const std::vector<std::string>& command) const
      -> tl::expected<DexCompileResult, DexCompilerError>;

  // Scan output directory for generated .dex files
  static auto
  scan_dex_files(const std::filesystem::path& output_dir) -> std::vector<std::filesystem::path>;

  // Validate D8/R8 tools are available
  auto validate_d8_tool() const -> tl::expected<void, DexCompilerError>;
  auto validate_r8_tool() const -> tl::expected<void, DexCompilerError>;

  // Sort .dex files deterministically (classes.dex, classes2.dex, ...)
  static auto sort_dex_files_deterministic(std::vector<std::filesystem::path>& dex_files) -> void;

  // Get D8 tool path from build tools
  auto get_d8_path() const -> std::optional<std::filesystem::path>;

  // Get R8 tool path from build tools
  auto get_r8_path() const -> std::optional<std::filesystem::path>;
};

// Helper functions for DEX merging
namespace dex_merge {

// Merge multiple DEX files into one
auto merge_dex_files(const std::vector<std::filesystem::path>& dex_files,
                     const std::filesystem::path& output_file, int min_api) -> bool;

// Split large DEX into multiple DEX files (multi-dex)
auto split_dex(const std::filesystem::path& input_dex, const std::filesystem::path& output_dir,
               const std::optional<std::filesystem::path>& main_dex_list) -> bool;

// Check if DEX file exceeds method limit (65536)
auto needs_multi_dex(const std::filesystem::path& dex_file) -> bool;

// Get method count in DEX file
auto get_method_count(const std::filesystem::path& dex_file) -> size_t;

} // namespace dex_merge

// Helper functions for incremental compilation
namespace dex_incremental {

// Compute hash of input files content
auto compute_input_hash(const std::vector<std::filesystem::path>& inputs) -> std::string;

// Check if any input file has changed since last compilation
auto has_input_changed(const std::vector<std::filesystem::path>& inputs,
                       const std::string& cached_hash) -> bool;

// Save compilation state for incremental builds
auto save_compilation_state(const std::filesystem::path& state_file, const std::string& config_hash,
                            const std::string& compilation_hash) -> bool;

// Load compilation state from previous build
auto load_compilation_state(const std::filesystem::path& state_file)
    -> std::optional<std::pair<std::string, std::string>>;

} // namespace dex_incremental

// Helper functions for ProGuard configuration
namespace proguard_utils {

// Load ProGuard rules from file
auto load_proguard_rules(const std::filesystem::path& config_path)
    -> tl::expected<std::vector<std::string>, DexCompilerError>;

// Generate default keep rules for Android
auto generate_default_keep_rules() -> std::vector<std::string>;

// Validate ProGuard configuration
auto validate_proguard_config(const ProguardConfig& config) -> bool;

// Merge multiple ProGuard configurations
auto merge_proguard_configs(const std::vector<ProguardConfig>& configs) -> ProguardConfig;

} // namespace proguard_utils

} // namespace horcrux::core
