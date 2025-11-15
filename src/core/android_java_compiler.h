// Horcrux - Android Java Compiler Rules
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#ifndef HORCRUX_CORE_ANDROID_JAVA_COMPILER_H_
#define HORCRUX_CORE_ANDROID_JAVA_COMPILER_H_

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "android_toolchain.h"

namespace horcrux::core {

/// @brief Error types for Java compilation
/// Used with tl::expected for recoverable error handling
enum class JavaCompilerError {
  CompilerNotFound,
  InvalidSourceFile,
  InvalidClasspath,
  CompilationFailed,
  InvalidConfiguration,
  IoError,
  UnknownError
};

// Convert error to string
auto to_string(JavaCompilerError error) -> std::string;

// Java source file representation
struct JavaSourceFile {
  std::filesystem::path path;
  std::string package_name;
  std::vector<std::string> imports;
  std::string content_hash; // SHA-256 hash for incremental compilation
};

// Java compilation configuration
struct JavaCompileConfig {
  // Source files to compile
  std::vector<JavaSourceFile> sources;

  // Classpath entries (JAR files and directories)
  std::vector<std::filesystem::path> classpath;

  // Output directory for compiled .class files
  std::filesystem::path output_dir;

  // Java source version (e.g., "17", "11")
  std::string source_version;

  // Java target version (e.g., "17", "11")
  std::string target_version;

  // Annotation processor classpath
  std::vector<std::filesystem::path> processor_path;

  // Annotation processor options
  std::vector<std::string> processor_options;

  // Additional javac options
  std::vector<std::string> javac_options;

  // Enable incremental compilation
  bool incremental = true;

  // Enable verbose output
  bool verbose = false;

  // Bootclasspath (e.g., android.jar for Android)
  std::optional<std::filesystem::path> bootclasspath;

  // Sandbox configuration
  bool enable_sandbox = false;  // Enable sandboxed execution
  std::optional<std::filesystem::path> sdk_path;  // Android SDK path for sandbox
};

// Java compilation result
struct JavaCompileResult {
  // Success status
  bool success;

  // Output directory containing compiled classes
  std::filesystem::path output_dir;

  // List of generated .class files
  std::vector<std::filesystem::path> class_files;

  // Compilation hash for caching
  std::string compilation_hash;

  // Compilation time in milliseconds
  std::chrono::milliseconds compilation_time;

  // Standard output from compiler
  std::string stdout_output;

  // Standard error from compiler
  std::string stderr_output;
};

// Android Java compiler rule
class AndroidJavaCompiler {
public:
  // Create compiler with toolchain
  explicit AndroidJavaCompiler(const AndroidToolchain& toolchain);

  // Compile Java sources
  auto
  compile(const JavaCompileConfig& config) -> tl::expected<JavaCompileResult, JavaCompilerError>;

  // Validate configuration
  static auto
  validate_config(const JavaCompileConfig& config) -> tl::expected<void, JavaCompilerError>;

  // Compute compilation hash for caching (Merkle signature)
  static auto compute_compilation_hash(const JavaCompileConfig& config) -> std::string;

  // Check if compilation is needed (incremental compilation)
  auto is_compilation_needed(const JavaCompileConfig& config,
                             const std::string& cached_hash) const -> bool;

  // Parse Java source file to extract metadata
  static auto parse_java_source(const std::filesystem::path& source_path)
      -> tl::expected<JavaSourceFile, JavaCompilerError>;

private:
  const AndroidToolchain& toolchain_;

  // Build javac command line
  auto build_javac_command(const JavaCompileConfig& config) const -> std::vector<std::string>;

  // Execute javac command
  auto execute_javac(const std::vector<std::string>& command) const
      -> tl::expected<JavaCompileResult, JavaCompilerError>;

  // Scan output directory for generated class files
  static auto
  scan_class_files(const std::filesystem::path& output_dir) -> std::vector<std::filesystem::path>;

  // Validate Java SDK is available
  auto validate_java_sdk() const -> tl::expected<void, JavaCompilerError>;
};

// Helper functions for classpath management
namespace java_classpath {

// Build classpath string from list of paths
auto build_classpath_string(const std::vector<std::filesystem::path>& classpath) -> std::string;

// Parse classpath string into list of paths
auto parse_classpath_string(const std::string& classpath_str) -> std::vector<std::filesystem::path>;

// Validate classpath entries exist
auto validate_classpath(const std::vector<std::filesystem::path>& classpath) -> bool;

// Get platform-specific path separator
auto get_path_separator() -> char;

} // namespace java_classpath

// Helper functions for incremental compilation
namespace java_incremental {

// Compute hash of source file content
auto compute_source_hash(const std::filesystem::path& source_path) -> std::string;

// Check if source file has changed since last compilation
auto has_source_changed(const std::filesystem::path& source_path,
                        const std::string& cached_hash) -> bool;

// Create compilation state file for incremental builds
auto save_compilation_state(const std::filesystem::path& state_file,
                            const JavaCompileConfig& config,
                            const std::string& compilation_hash) -> bool;

// Load compilation state from previous build
auto load_compilation_state(const std::filesystem::path& state_file) -> std::optional<std::string>;

} // namespace java_incremental

} // namespace horcrux::core

#endif // HORCRUX_CORE_ANDROID_JAVA_COMPILER_H_
