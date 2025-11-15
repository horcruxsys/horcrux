// Horcrux - Android Java Compiler Rules Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_java_compiler.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>

#include "local_cache.h" // For SHA-256 hashing

namespace horcrux::core {

namespace {

// Execute command and capture output
auto execute_command(const std::vector<std::string>& command)
    -> std::tuple<int, std::string, std::string> {
  // Build command string
  std::ostringstream cmd_stream;
  for (size_t i = 0; i < command.size(); ++i) {
    if (i > 0) {
      cmd_stream << " ";
    }
    // Quote arguments with spaces
    if (command[i].find(' ') != std::string::npos) {
      cmd_stream << "\"" << command[i] << "\"";
    } else {
      cmd_stream << command[i];
    }
  }

  std::string cmd = cmd_stream.str();
  cmd += " 2>&1"; // Redirect stderr to stdout

  std::array<char, 128> buffer{};
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

  if (!pipe) {
    return {-1, "", "Failed to execute command"};
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  int exit_code = pclose(pipe.release());
  return {exit_code, result, ""};
}

// Read file content
auto read_file_content(const std::filesystem::path& path) -> std::optional<std::string> {
  std::ifstream file(path);
  if (!file.is_open()) {
    return std::nullopt;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

} // anonymous namespace

auto to_string(JavaCompilerError error) -> std::string {
  switch (error) {
  case JavaCompilerError::CompilerNotFound:
    return "Java compiler (javac) not found. Ensure JAVA_HOME is set correctly.";
  case JavaCompilerError::InvalidSourceFile:
    return "Invalid Java source file. File does not exist or is not readable.";
  case JavaCompilerError::InvalidClasspath:
    return "Invalid classpath. One or more classpath entries do not exist.";
  case JavaCompilerError::CompilationFailed:
    return "Java compilation failed. Check compiler output for errors.";
  case JavaCompilerError::InvalidConfiguration:
    return "Invalid compilation configuration. Check source version and target version.";
  case JavaCompilerError::IoError:
    return "I/O error during compilation.";
  case JavaCompilerError::UnknownError:
    return "Unknown error during Java compilation.";
  }
  return "Unknown error";
}

AndroidJavaCompiler::AndroidJavaCompiler(const AndroidToolchain& toolchain)
    : toolchain_(toolchain) {}

auto AndroidJavaCompiler::compile(const JavaCompileConfig& config)
    -> tl::expected<JavaCompileResult, JavaCompilerError> {
  // Validate configuration
  auto validation = validate_config(config);
  if (!validation) {
    return tl::unexpected(validation.error());
  }

  // Validate Java SDK
  auto java_validation = validate_java_sdk();
  if (!java_validation) {
    return tl::unexpected(java_validation.error());
  }

  // Check if incremental compilation is possible
  std::string compilation_hash = compute_compilation_hash(config);
  if (config.incremental) {
    std::filesystem::path state_file = config.output_dir / ".horcrux_java_state";
    if (std::filesystem::exists(state_file)) {
      auto cached_hash = java_incremental::load_compilation_state(state_file);
      if (cached_hash && *cached_hash == compilation_hash) {
        // No compilation needed, return cached result
        JavaCompileResult result;
        result.success = true;
        result.output_dir = config.output_dir;
        result.class_files = scan_class_files(config.output_dir);
        result.compilation_hash = compilation_hash;
        result.compilation_time = std::chrono::milliseconds(0);
        result.stdout_output = "Compilation skipped (up-to-date)";
        return result;
      }
    }
  }

  // Create output directory
  std::error_code ec;
  std::filesystem::create_directories(config.output_dir, ec);
  if (ec) {
    return tl::unexpected(JavaCompilerError::IoError);
  }

  // Build javac command
  auto command = build_javac_command(config);

  // Execute compilation
  auto start_time = std::chrono::steady_clock::now();
  auto [exit_code, stdout_str, stderr_str] = execute_command(command);
  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Build result
  JavaCompileResult result;
  result.success = (exit_code == 0);
  result.output_dir = config.output_dir;
  result.compilation_hash = compilation_hash;
  result.compilation_time = duration;
  result.stdout_output = stdout_str;
  result.stderr_output = stderr_str;

  if (!result.success) {
    return tl::unexpected(JavaCompilerError::CompilationFailed);
  }

  // Scan for generated class files
  result.class_files = scan_class_files(config.output_dir);

  // Save compilation state for incremental builds
  if (config.incremental) {
    std::filesystem::path state_file = config.output_dir / ".horcrux_java_state";
    java_incremental::save_compilation_state(state_file, config, compilation_hash);
  }

  return result;
}

auto AndroidJavaCompiler::validate_config(const JavaCompileConfig& config)
    -> tl::expected<void, JavaCompilerError> {
  // Check sources
  if (config.sources.empty()) {
    return tl::unexpected(JavaCompilerError::InvalidConfiguration);
  }

  // Validate source files exist
  for (const auto& source : config.sources) {
    if (!std::filesystem::exists(source.path)) {
      return tl::unexpected(JavaCompilerError::InvalidSourceFile);
    }
  }

  // Validate classpath if specified
  if (!config.classpath.empty() && !java_classpath::validate_classpath(config.classpath)) {
    return tl::unexpected(JavaCompilerError::InvalidClasspath);
  }

  return {};
}

auto AndroidJavaCompiler::compute_compilation_hash(const JavaCompileConfig& config) -> std::string {
  std::ostringstream ss;

  // Hash source files
  for (const auto& source : config.sources) {
    ss << "source:" << source.path.string() << ":" << source.content_hash << "\n";
  }

  // Hash classpath
  for (const auto& cp : config.classpath) {
    ss << "classpath:" << cp.string() << "\n";
  }

  // Hash bootclasspath
  if (config.bootclasspath) {
    ss << "bootclasspath:" << config.bootclasspath->string() << "\n";
  }

  // Hash versions
  ss << "source_version:" << config.source_version << "\n";
  ss << "target_version:" << config.target_version << "\n";

  // Hash processor options
  for (const auto& opt : config.processor_options) {
    ss << "processor_option:" << opt << "\n";
  }

  // Hash javac options
  for (const auto& opt : config.javac_options) {
    ss << "javac_option:" << opt << "\n";
  }

  // Compute SHA-256 hash
  std::string data_str = ss.str();
  std::vector<uint8_t> data(data_str.begin(), data_str.end());
  auto hash = compute_sha256(data);
  return hash_to_string(hash);
}

auto AndroidJavaCompiler::is_compilation_needed(const JavaCompileConfig& config,
                                               const std::string& cached_hash) const -> bool {
  std::string current_hash = compute_compilation_hash(config);
  return current_hash != cached_hash;
}

auto AndroidJavaCompiler::parse_java_source(const std::filesystem::path& source_path)
    -> tl::expected<JavaSourceFile, JavaCompilerError> {
  auto content = read_file_content(source_path);
  if (!content) {
    return tl::unexpected(JavaCompilerError::InvalidSourceFile);
  }

  JavaSourceFile source_file;
  source_file.path = source_path;

  // Extract package name
  std::regex package_regex(R"(package\s+([\w.]+)\s*;)");
  std::smatch package_match;
  if (std::regex_search(*content, package_match, package_regex)) {
    source_file.package_name = package_match[1].str();
  }

  // Extract imports
  std::regex import_regex(R"(import\s+([\w.*]+)\s*;)");
  auto imports_begin = std::sregex_iterator(content->begin(), content->end(), import_regex);
  auto imports_end = std::sregex_iterator();
  for (auto it = imports_begin; it != imports_end; ++it) {
    source_file.imports.push_back((*it)[1].str());
  }

  // Compute content hash
  source_file.content_hash = java_incremental::compute_source_hash(source_path);

  return source_file;
}

auto AndroidJavaCompiler::build_javac_command(const JavaCompileConfig& config) const
    -> std::vector<std::string> {
  std::vector<std::string> command;

  // Add javac executable
  if (toolchain_.java_sdk) {
    command.push_back(toolchain_.java_sdk->javac_path.string());
  } else {
    command.push_back("javac"); // Fallback to PATH
  }

  // Add source and target versions
  if (!config.source_version.empty()) {
    command.push_back("-source");
    command.push_back(config.source_version);
  }

  if (!config.target_version.empty()) {
    command.push_back("-target");
    command.push_back(config.target_version);
  }

  // Add bootclasspath (for Android)
  if (config.bootclasspath) {
    command.push_back("-bootclasspath");
    command.push_back(config.bootclasspath->string());
  }

  // Add classpath
  if (!config.classpath.empty()) {
    command.push_back("-classpath");
    command.push_back(java_classpath::build_classpath_string(config.classpath));
  }

  // Add output directory
  command.push_back("-d");
  command.push_back(config.output_dir.string());

  // Add annotation processor path
  if (!config.processor_path.empty()) {
    command.push_back("-processorpath");
    command.push_back(java_classpath::build_classpath_string(config.processor_path));
  }

  // Add annotation processor options
  for (const auto& opt : config.processor_options) {
    command.push_back("-A" + opt);
  }

  // Add additional javac options
  for (const auto& opt : config.javac_options) {
    command.push_back(opt);
  }

  // Add verbose flag
  if (config.verbose) {
    command.push_back("-verbose");
  }

  // Add source files (deterministic ordering)
  std::vector<std::string> source_paths;
  for (const auto& source : config.sources) {
    source_paths.push_back(source.path.string());
  }
  std::sort(source_paths.begin(), source_paths.end());
  command.insert(command.end(), source_paths.begin(), source_paths.end());

  return command;
}

auto AndroidJavaCompiler::execute_javac(const std::vector<std::string>& command) const
    -> tl::expected<JavaCompileResult, JavaCompilerError> {
  auto [exit_code, stdout_str, stderr_str] = execute_command(command);

  JavaCompileResult result;
  result.success = (exit_code == 0);
  result.stdout_output = stdout_str;
  result.stderr_output = stderr_str;

  if (!result.success) {
    return tl::unexpected(JavaCompilerError::CompilationFailed);
  }

  return result;
}

auto AndroidJavaCompiler::scan_class_files(const std::filesystem::path& output_dir)
    -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> class_files;

  if (!std::filesystem::exists(output_dir)) {
    return class_files;
  }

  try {
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(output_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".class") {
        class_files.push_back(entry.path());
      }
    }
  } catch (const std::filesystem::filesystem_error&) {
    // Ignore errors, return what we found
  }

  // Sort for deterministic behavior
  std::sort(class_files.begin(), class_files.end());

  return class_files;
}

auto AndroidJavaCompiler::validate_java_sdk() const -> tl::expected<void, JavaCompilerError> {
  if (!toolchain_.java_sdk) {
    return tl::unexpected(JavaCompilerError::CompilerNotFound);
  }

  if (!std::filesystem::exists(toolchain_.java_sdk->javac_path)) {
    return tl::unexpected(JavaCompilerError::CompilerNotFound);
  }

  return {};
}

// Classpath helper implementations
namespace java_classpath {

auto build_classpath_string(const std::vector<std::filesystem::path>& classpath) -> std::string {
  if (classpath.empty()) {
    return "";
  }

  char separator = get_path_separator();
  std::ostringstream ss;

  for (size_t i = 0; i < classpath.size(); ++i) {
    if (i > 0) {
      ss << separator;
    }
    ss << classpath[i].string();
  }

  return ss.str();
}

auto parse_classpath_string(const std::string& classpath_str)
    -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> result;
  char separator = get_path_separator();

  std::istringstream ss(classpath_str);
  std::string path;

  while (std::getline(ss, path, separator)) {
    if (!path.empty()) {
      result.push_back(path);
    }
  }

  return result;
}

auto validate_classpath(const std::vector<std::filesystem::path>& classpath) -> bool {
  for (const auto& entry : classpath) {
    if (!std::filesystem::exists(entry)) {
      return false;
    }
  }
  return true;
}

auto get_path_separator() -> char {
#ifdef _WIN32
  return ';';
#else
  return ':';
#endif
}

} // namespace java_classpath

// Incremental compilation helper implementations
namespace java_incremental {

auto compute_source_hash(const std::filesystem::path& source_path) -> std::string {
  auto content = read_file_content(source_path);
  if (!content) {
    return "";
  }

  std::vector<uint8_t> data(content->begin(), content->end());
  auto hash = compute_sha256(data);
  return hash_to_string(hash);
}

auto has_source_changed(const std::filesystem::path& source_path,
                       const std::string& cached_hash) -> bool {
  std::string current_hash = compute_source_hash(source_path);
  return current_hash != cached_hash;
}

auto save_compilation_state(const std::filesystem::path& state_file,
                           const JavaCompileConfig& /*config*/,
                           const std::string& compilation_hash) -> bool {
  std::ofstream file(state_file);
  if (!file.is_open()) {
    return false;
  }

  file << compilation_hash << "\n";
  return true;
}

auto load_compilation_state(const std::filesystem::path& state_file)
    -> std::optional<std::string> {
  std::ifstream file(state_file);
  if (!file.is_open()) {
    return std::nullopt;
  }

  std::string hash;
  std::getline(file, hash);

  if (hash.empty()) {
    return std::nullopt;
  }

  return hash;
}

} // namespace java_incremental

} // namespace horcrux::core
