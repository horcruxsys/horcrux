// Horcrux - Android Kotlin Compiler Rules
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#ifndef HORCRUX_CORE_ANDROID_KOTLIN_COMPILER_H_
#define HORCRUX_CORE_ANDROID_KOTLIN_COMPILER_H_

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "android_toolchain.h"

namespace horcrux::core {

// Error types for Kotlin compilation
enum class KotlinCompilerError {
  CompilerNotFound,
  InvalidSourceFile,
  InvalidClasspath,
  CompilationFailed,
  InvalidConfiguration,
  IoError,
  UnknownError
};

// Convert error to string
auto to_string(KotlinCompilerError error) -> std::string;

// Kotlin source file representation
struct KotlinSourceFile {
  std::filesystem::path path;
  std::string package_name;
  std::vector<std::string> imports;
  std::string content_hash; // SHA-256 hash for incremental compilation
};

// Kotlin compilation configuration
struct KotlinCompileConfig {
  // Source files to compile (Kotlin and/or Java)
  std::vector<KotlinSourceFile> kotlin_sources;
  std::vector<std::filesystem::path> java_sources;

  // Classpath entries (JAR files and directories)
  std::vector<std::filesystem::path> classpath;

  // Output directory for compiled .class files
  std::filesystem::path output_dir;

  // Kotlin language version (e.g., "1.9", "1.8")
  std::string language_version;

  // JVM target version (e.g., "17", "11")
  std::string jvm_target;

  // API version (e.g., "1.9", "1.8")
  std::string api_version;

  // Plugin classpath (for KAPT, KSP)
  std::vector<std::filesystem::path> plugin_classpath;

  // Plugin options
  std::vector<std::string> plugin_options;

  // Additional kotlinc options
  std::vector<std::string> kotlinc_options;

  // Enable incremental compilation
  bool incremental = true;

  // Enable verbose output
  bool verbose = false;

  // Bootclasspath (e.g., android.jar for Android)
  std::optional<std::filesystem::path> bootclasspath;

  // KAPT annotation processor configuration
  struct KaptConfig {
    bool enabled = false;
    std::vector<std::filesystem::path> processor_classpath;
    std::filesystem::path generated_sources_dir;
    std::filesystem::path generated_stubs_dir;
    std::vector<std::string> processor_options;
  };
  std::optional<KaptConfig> kapt_config;

  // KSP (Kotlin Symbol Processing) configuration
  struct KspConfig {
    bool enabled = false;
    std::vector<std::filesystem::path> processor_classpath;
    std::filesystem::path output_dir;
    std::vector<std::string> processor_options;
  };
  std::optional<KspConfig> ksp_config;
};

// Kotlin compilation result
struct KotlinCompileResult {
  // Success status
  bool success;

  // Output directory containing compiled classes
  std::filesystem::path output_dir;

  // List of generated .class files
  std::vector<std::filesystem::path> class_files;

  // Generated source files (from KAPT/KSP)
  std::vector<std::filesystem::path> generated_sources;

  // Compilation hash for caching
  std::string compilation_hash;

  // Compilation time in milliseconds
  std::chrono::milliseconds compilation_time;

  // Standard output from compiler
  std::string stdout_output;

  // Standard error from compiler
  std::string stderr_output;
};

// Android Kotlin compiler rule
class AndroidKotlinCompiler {
public:
  // Create compiler with toolchain
  explicit AndroidKotlinCompiler(const AndroidToolchain& toolchain);

  // Compile Kotlin sources
  auto compile(const KotlinCompileConfig& config)
      -> tl::expected<KotlinCompileResult, KotlinCompilerError>;

  // Validate configuration
  static auto
  validate_config(const KotlinCompileConfig& config) -> tl::expected<void, KotlinCompilerError>;

  // Compute compilation hash for caching (Merkle signature)
  static auto compute_compilation_hash(const KotlinCompileConfig& config) -> std::string;

  // Check if compilation is needed (incremental compilation)
  auto is_compilation_needed(const KotlinCompileConfig& config,
                            const std::string& cached_hash) const -> bool;

  // Parse Kotlin source file to extract metadata
  static auto parse_kotlin_source(const std::filesystem::path& source_path)
      -> tl::expected<KotlinSourceFile, KotlinCompilerError>;

  // Set Kotlin compiler path (for custom installations)
  void set_kotlinc_path(const std::filesystem::path& kotlinc_path);

private:
  const AndroidToolchain& toolchain_;
  std::optional<std::filesystem::path> kotlinc_path_;

  // Build kotlinc command line
  auto build_kotlinc_command(const KotlinCompileConfig& config) const
      -> std::vector<std::string>;

  // Execute kotlinc command
  auto execute_kotlinc(const std::vector<std::string>& command) const
      -> tl::expected<KotlinCompileResult, KotlinCompilerError>;

  // Run KAPT annotation processing
  auto run_kapt(const KotlinCompileConfig& config)
      -> tl::expected<void, KotlinCompilerError>;

  // Run KSP (Kotlin Symbol Processing)
  auto run_ksp(const KotlinCompileConfig& config)
      -> tl::expected<void, KotlinCompilerError>;

  // Scan output directory for generated class files
  static auto scan_class_files(const std::filesystem::path& output_dir)
      -> std::vector<std::filesystem::path>;

  // Scan for generated source files
  static auto scan_generated_sources(const std::filesystem::path& generated_dir)
      -> std::vector<std::filesystem::path>;

  // Detect kotlinc compiler
  auto detect_kotlinc() const -> std::optional<std::filesystem::path>;

  // Validate Kotlin compiler is available
  auto validate_kotlin_compiler() const -> tl::expected<void, KotlinCompilerError>;
};

// Helper functions for Kotlin/Java interop
namespace kotlin_java_interop {

// Check if a source file is Kotlin or Java
auto is_kotlin_file(const std::filesystem::path& path) -> bool;
auto is_java_file(const std::filesystem::path& path) -> bool;

// Separate mixed source files into Kotlin and Java
auto separate_sources(const std::vector<std::filesystem::path>& sources)
    -> std::pair<std::vector<std::filesystem::path>, std::vector<std::filesystem::path>>;

// Validate mixed Kotlin/Java compilation order
auto validate_compilation_order(const std::vector<std::filesystem::path>& kotlin_sources,
                               const std::vector<std::filesystem::path>& java_sources) -> bool;

} // namespace kotlin_java_interop

// Helper functions for incremental Kotlin compilation
namespace kotlin_incremental {

// Compute hash of source file content
auto compute_source_hash(const std::filesystem::path& source_path) -> std::string;

// Check if source file has changed since last compilation
auto has_source_changed(const std::filesystem::path& source_path,
                       const std::string& cached_hash) -> bool;

// Create compilation state file for incremental builds
auto save_compilation_state(const std::filesystem::path& state_file,
                           const KotlinCompileConfig& config,
                           const std::string& compilation_hash) -> bool;

// Load compilation state from previous build
auto load_compilation_state(const std::filesystem::path& state_file)
    -> std::optional<std::string>;

} // namespace kotlin_incremental

} // namespace horcrux::core

#endif // HORCRUX_CORE_ANDROID_KOTLIN_COMPILER_H_
