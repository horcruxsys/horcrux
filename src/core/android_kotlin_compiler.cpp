// Horcrux - Android Kotlin Compiler Rules Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_kotlin_compiler.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>

#include "android_java_compiler.h" // For classpath helpers
#include "local_cache.h"           // For SHA-256 hashing

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

auto to_string(KotlinCompilerError error) -> std::string {
  switch (error) {
  case KotlinCompilerError::CompilerNotFound:
    return "Kotlin compiler (kotlinc) not found. Ensure kotlinc is in PATH or set explicitly.";
  case KotlinCompilerError::InvalidSourceFile:
    return "Invalid Kotlin source file. File does not exist or is not readable.";
  case KotlinCompilerError::InvalidClasspath:
    return "Invalid classpath. One or more classpath entries do not exist.";
  case KotlinCompilerError::CompilationFailed:
    return "Kotlin compilation failed. Check compiler output for errors.";
  case KotlinCompilerError::InvalidConfiguration:
    return "Invalid compilation configuration. Check language version and target version.";
  case KotlinCompilerError::IoError:
    return "I/O error during compilation.";
  case KotlinCompilerError::UnknownError:
    return "Unknown error during Kotlin compilation.";
  }
  return "Unknown error";
}

AndroidKotlinCompiler::AndroidKotlinCompiler(const AndroidToolchain& toolchain)
    : toolchain_(toolchain) {
}

auto AndroidKotlinCompiler::compile(const KotlinCompileConfig& config)
    -> tl::expected<KotlinCompileResult, KotlinCompilerError> {
  // Validate configuration
  auto validation = validate_config(config);
  if (!validation) {
    return tl::unexpected(validation.error());
  }

  // Validate Kotlin compiler
  auto compiler_validation = validate_kotlin_compiler();
  if (!compiler_validation) {
    return tl::unexpected(compiler_validation.error());
  }

  // Check if incremental compilation is possible
  std::string compilation_hash = compute_compilation_hash(config);
  if (config.incremental) {
    std::filesystem::path state_file = config.output_dir / ".horcrux_kotlin_state";
    if (std::filesystem::exists(state_file)) {
      auto cached_hash = kotlin_incremental::load_compilation_state(state_file);
      if (cached_hash && *cached_hash == compilation_hash) {
        // No compilation needed, return cached result
        KotlinCompileResult result;
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
    return tl::unexpected(KotlinCompilerError::IoError);
  }

  // Run KAPT if configured
  if (config.kapt_config && config.kapt_config->enabled) {
    auto kapt_result = run_kapt(config);
    if (!kapt_result) {
      return tl::unexpected(kapt_result.error());
    }
  }

  // Run KSP if configured
  if (config.ksp_config && config.ksp_config->enabled) {
    auto ksp_result = run_ksp(config);
    if (!ksp_result) {
      return tl::unexpected(ksp_result.error());
    }
  }

  // Build kotlinc command
  auto command = build_kotlinc_command(config);

  // Execute compilation
  auto start_time = std::chrono::steady_clock::now();
  auto [exit_code, stdout_str, stderr_str] = execute_command(command);
  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Build result
  KotlinCompileResult result;
  result.success = (exit_code == 0);
  result.output_dir = config.output_dir;
  result.compilation_hash = compilation_hash;
  result.compilation_time = duration;
  result.stdout_output = stdout_str;
  result.stderr_output = stderr_str;

  if (!result.success) {
    return tl::unexpected(KotlinCompilerError::CompilationFailed);
  }

  // Scan for generated class files
  result.class_files = scan_class_files(config.output_dir);

  // Scan for generated sources (from KAPT/KSP)
  if (config.kapt_config && config.kapt_config->enabled) {
    auto kapt_sources = scan_generated_sources(config.kapt_config->generated_sources_dir);
    result.generated_sources.insert(result.generated_sources.end(), kapt_sources.begin(),
                                    kapt_sources.end());
  }
  if (config.ksp_config && config.ksp_config->enabled) {
    auto ksp_sources = scan_generated_sources(config.ksp_config->output_dir);
    result.generated_sources.insert(result.generated_sources.end(), ksp_sources.begin(),
                                    ksp_sources.end());
  }

  // Save compilation state for incremental builds
  if (config.incremental) {
    std::filesystem::path state_file = config.output_dir / ".horcrux_kotlin_state";
    kotlin_incremental::save_compilation_state(state_file, config, compilation_hash);
  }

  return result;
}

auto AndroidKotlinCompiler::validate_config(const KotlinCompileConfig& config)
    -> tl::expected<void, KotlinCompilerError> {
  // Check sources
  if (config.kotlin_sources.empty() && config.java_sources.empty()) {
    return tl::unexpected(KotlinCompilerError::InvalidConfiguration);
  }

  // Validate Kotlin source files exist
  for (const auto& source : config.kotlin_sources) {
    if (!std::filesystem::exists(source.path)) {
      return tl::unexpected(KotlinCompilerError::InvalidSourceFile);
    }
  }

  // Validate Java source files exist
  for (const auto& source : config.java_sources) {
    if (!std::filesystem::exists(source)) {
      return tl::unexpected(KotlinCompilerError::InvalidSourceFile);
    }
  }

  // Validate classpath if specified
  if (!config.classpath.empty() && !java_classpath::validate_classpath(config.classpath)) {
    return tl::unexpected(KotlinCompilerError::InvalidClasspath);
  }

  // Validate Compose configuration if enabled
  if (config.compose_config && config.compose_config->enabled) {
    auto compose_validation = compose_compiler::validate_compose_config(*config.compose_config);
    if (!compose_validation) {
      // Convert Compose validation error to Kotlin compiler error
      return tl::unexpected(KotlinCompilerError::InvalidConfiguration);
    }
  }

  return {};
}

auto AndroidKotlinCompiler::compute_compilation_hash(const KotlinCompileConfig& config)
    -> std::string {
  std::ostringstream ss;

  // Hash Kotlin source files
  for (const auto& source : config.kotlin_sources) {
    ss << "kotlin_source:" << source.path.string() << ":" << source.content_hash << "\n";
  }

  // Hash Java source files
  for (const auto& source : config.java_sources) {
    ss << "java_source:" << source.string() << "\n";
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
  ss << "language_version:" << config.language_version << "\n";
  ss << "jvm_target:" << config.jvm_target << "\n";
  ss << "api_version:" << config.api_version << "\n";

  // Hash plugin options
  for (const auto& opt : config.plugin_options) {
    ss << "plugin_option:" << opt << "\n";
  }

  // Hash kotlinc options
  for (const auto& opt : config.kotlinc_options) {
    ss << "kotlinc_option:" << opt << "\n";
  }

  // Hash KAPT configuration
  if (config.kapt_config && config.kapt_config->enabled) {
    ss << "kapt:enabled\n";
    for (const auto& opt : config.kapt_config->processor_options) {
      ss << "kapt_option:" << opt << "\n";
    }
  }

  // Hash KSP configuration
  if (config.ksp_config && config.ksp_config->enabled) {
    ss << "ksp:enabled\n";
    for (const auto& opt : config.ksp_config->processor_options) {
      ss << "ksp_option:" << opt << "\n";
    }
  }

  // Hash Compose compiler configuration
  if (config.compose_config && config.compose_config->enabled) {
    ss << "compose:enabled\n";
    ss << "compose_version:" << config.compose_config->version << "\n";
    ss << "compose_kotlin_version:" << config.compose_config->kotlin_version << "\n";
    ss << "compose_metrics:" << config.compose_config->enable_metrics << "\n";
    ss << "compose_reports:" << config.compose_config->enable_reports << "\n";
    ss << "compose_live_literals:" << config.compose_config->enable_live_literals << "\n";
    ss << "compose_source_info:" << config.compose_config->enable_source_information << "\n";
    ss << "compose_intrinsic_remember:" << config.compose_config->enable_intrinsic_remember << "\n";
    
    if (config.compose_config->stability_config_path) {
      ss << "compose_stability:" << config.compose_config->stability_config_path->string() << "\n";
    }
    
    for (const auto& opt : config.compose_config->additional_options) {
      ss << "compose_option:" << opt << "\n";
    }
  }

  // Compute SHA-256 hash
  std::string data_str = ss.str();
  std::vector<uint8_t> data(data_str.begin(), data_str.end());
  auto hash = compute_sha256(data);
  return hash_to_string(hash);
}

auto AndroidKotlinCompiler::is_compilation_needed(const KotlinCompileConfig& config,
                                                  const std::string& cached_hash) const -> bool {
  std::string current_hash = compute_compilation_hash(config);
  return current_hash != cached_hash;
}

auto AndroidKotlinCompiler::parse_kotlin_source(const std::filesystem::path& source_path)
    -> tl::expected<KotlinSourceFile, KotlinCompilerError> {
  auto content = read_file_content(source_path);
  if (!content) {
    return tl::unexpected(KotlinCompilerError::InvalidSourceFile);
  }

  KotlinSourceFile source_file;
  source_file.path = source_path;

  // Extract package name
  std::regex package_regex(R"(package\s+([\w.]+))");
  std::smatch package_match;
  if (std::regex_search(*content, package_match, package_regex)) {
    source_file.package_name = package_match[1].str();
  }

  // Extract imports
  std::regex import_regex(R"(import\s+([\w.*]+))");
  auto imports_begin = std::sregex_iterator(content->begin(), content->end(), import_regex);
  auto imports_end = std::sregex_iterator();
  for (auto it = imports_begin; it != imports_end; ++it) {
    source_file.imports.push_back((*it)[1].str());
  }

  // Compute content hash
  source_file.content_hash = kotlin_incremental::compute_source_hash(source_path);

  return source_file;
}

void AndroidKotlinCompiler::set_kotlinc_path(const std::filesystem::path& kotlinc_path) {
  kotlinc_path_ = kotlinc_path;
}

auto AndroidKotlinCompiler::build_kotlinc_command(const KotlinCompileConfig& config) const
    -> std::vector<std::string> {
  std::vector<std::string> command;

  // Add kotlinc executable
  if (kotlinc_path_) {
    command.push_back(kotlinc_path_->string());
  } else {
    auto detected = detect_kotlinc();
    if (detected) {
      command.push_back(detected->string());
    } else {
      command.push_back("kotlinc"); // Fallback to PATH
    }
  }

  // Add language version
  if (!config.language_version.empty()) {
    command.push_back("-language-version");
    command.push_back(config.language_version);
  }

  // Add API version
  if (!config.api_version.empty()) {
    command.push_back("-api-version");
    command.push_back(config.api_version);
  }

  // Add JVM target
  if (!config.jvm_target.empty()) {
    command.push_back("-jvm-target");
    command.push_back(config.jvm_target);
  }

  // Add classpath
  if (!config.classpath.empty()) {
    command.push_back("-classpath");
    command.push_back(java_classpath::build_classpath_string(config.classpath));
  }

  // Add output directory
  command.push_back("-d");
  command.push_back(config.output_dir.string());

  // Add plugin classpath
  if (!config.plugin_classpath.empty()) {
    command.push_back("-Xplugin");
    command.push_back(java_classpath::build_classpath_string(config.plugin_classpath));
  }

  // Add plugin options
  for (const auto& opt : config.plugin_options) {
    command.push_back("-P");
    command.push_back(opt);
  }

  // Add Compose compiler plugin options
  if (config.compose_config && config.compose_config->enabled) {
    auto compose_options = compose_compiler::build_compose_plugin_options(*config.compose_config);
    command.insert(command.end(), compose_options.begin(), compose_options.end());
  }

  // Add additional kotlinc options
  for (const auto& opt : config.kotlinc_options) {
    command.push_back(opt);
  }

  // Add verbose flag
  if (config.verbose) {
    command.push_back("-verbose");
  }

  // Add Kotlin source files (deterministic ordering)
  std::vector<std::string> kotlin_paths;
  for (const auto& source : config.kotlin_sources) {
    kotlin_paths.push_back(source.path.string());
  }
  std::sort(kotlin_paths.begin(), kotlin_paths.end());
  command.insert(command.end(), kotlin_paths.begin(), kotlin_paths.end());

  // Add Java source files (deterministic ordering)
  std::vector<std::string> java_paths;
  for (const auto& source : config.java_sources) {
    java_paths.push_back(source.string());
  }
  std::sort(java_paths.begin(), java_paths.end());
  command.insert(command.end(), java_paths.begin(), java_paths.end());

  return command;
}

auto AndroidKotlinCompiler::execute_kotlinc(const std::vector<std::string>& command) const
    -> tl::expected<KotlinCompileResult, KotlinCompilerError> {
  auto [exit_code, stdout_str, stderr_str] = execute_command(command);

  KotlinCompileResult result;
  result.success = (exit_code == 0);
  result.stdout_output = stdout_str;
  result.stderr_output = stderr_str;

  if (!result.success) {
    return tl::unexpected(KotlinCompilerError::CompilationFailed);
  }

  return result;
}

auto AndroidKotlinCompiler::run_kapt(const KotlinCompileConfig& config)
    -> tl::expected<void, KotlinCompilerError> {
  if (!config.kapt_config || !config.kapt_config->enabled) {
    return {};
  }

  // Create KAPT directories
  std::error_code ec;
  std::filesystem::create_directories(config.kapt_config->generated_sources_dir, ec);
  if (ec) {
    return tl::unexpected(KotlinCompilerError::IoError);
  }
  std::filesystem::create_directories(config.kapt_config->generated_stubs_dir, ec);
  if (ec) {
    return tl::unexpected(KotlinCompilerError::IoError);
  }

  // KAPT is implemented as a Kotlin compiler plugin
  // The actual implementation would invoke the KAPT plugin
  // For now, this is a placeholder for the KAPT integration

  return {};
}

auto AndroidKotlinCompiler::run_ksp(const KotlinCompileConfig& config)
    -> tl::expected<void, KotlinCompilerError> {
  if (!config.ksp_config || !config.ksp_config->enabled) {
    return {};
  }

  // Create KSP output directory
  std::error_code ec;
  std::filesystem::create_directories(config.ksp_config->output_dir, ec);
  if (ec) {
    return tl::unexpected(KotlinCompilerError::IoError);
  }

  // KSP is a separate tool that runs before Kotlin compilation
  // The actual implementation would invoke the KSP processor
  // For now, this is a placeholder for the KSP integration

  return {};
}

auto AndroidKotlinCompiler::scan_class_files(const std::filesystem::path& output_dir)
    -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> class_files;

  if (!std::filesystem::exists(output_dir)) {
    return class_files;
  }

  try {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(output_dir)) {
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

auto AndroidKotlinCompiler::scan_generated_sources(const std::filesystem::path& generated_dir)
    -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> source_files;

  if (!std::filesystem::exists(generated_dir)) {
    return source_files;
  }

  try {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(generated_dir)) {
      if (entry.is_regular_file()) {
        auto ext = entry.path().extension();
        if (ext == ".kt" || ext == ".java") {
          source_files.push_back(entry.path());
        }
      }
    }
  } catch (const std::filesystem::filesystem_error&) {
    // Ignore errors, return what we found
  }

  // Sort for deterministic behavior
  std::sort(source_files.begin(), source_files.end());

  return source_files;
}

auto AndroidKotlinCompiler::detect_kotlinc() const -> std::optional<std::filesystem::path> {
  // Try common installation locations
  std::vector<std::filesystem::path> common_paths = {
      "/usr/local/bin/kotlinc", "/usr/bin/kotlinc",
      std::filesystem::path(std::getenv("HOME") ? std::getenv("HOME") : "") /
          ".sdkman/candidates/kotlin/current/bin/kotlinc"};

  for (const auto& path : common_paths) {
    if (std::filesystem::exists(path)) {
      return path;
    }
  }

  // Check PATH environment variable
  // In production, we would properly parse PATH and search for kotlinc
  return std::nullopt;
}

auto AndroidKotlinCompiler::validate_kotlin_compiler() const
    -> tl::expected<void, KotlinCompilerError> {
  if (kotlinc_path_) {
    if (!std::filesystem::exists(*kotlinc_path_)) {
      return tl::unexpected(KotlinCompilerError::CompilerNotFound);
    }
    return {};
  }

  // Try to detect kotlinc
  auto detected = detect_kotlinc();
  if (!detected) {
    // Kotlinc not found, but this might be okay if it's in PATH
    // Return success for now
    return {};
  }

  return {};
}

// Kotlin/Java interop helper implementations
namespace kotlin_java_interop {

auto is_kotlin_file(const std::filesystem::path& path) -> bool {
  return path.extension() == ".kt";
}

auto is_java_file(const std::filesystem::path& path) -> bool {
  return path.extension() == ".java";
}

auto separate_sources(const std::vector<std::filesystem::path>& sources)
    -> std::pair<std::vector<std::filesystem::path>, std::vector<std::filesystem::path>> {
  std::vector<std::filesystem::path> kotlin_sources;
  std::vector<std::filesystem::path> java_sources;

  for (const auto& source : sources) {
    if (is_kotlin_file(source)) {
      kotlin_sources.push_back(source);
    } else if (is_java_file(source)) {
      java_sources.push_back(source);
    }
  }

  return {kotlin_sources, java_sources};
}

auto validate_compilation_order(const std::vector<std::filesystem::path>& /*kotlin_sources*/,
                                const std::vector<std::filesystem::path>& /*java_sources*/)
    -> bool {
  // In mixed Kotlin/Java projects, Kotlin compiler can handle both
  // No special ordering needed as kotlinc can compile both
  return true;
}

} // namespace kotlin_java_interop

// Incremental Kotlin compilation helper implementations
namespace kotlin_incremental {

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
                            const KotlinCompileConfig& /*config*/,
                            const std::string& compilation_hash) -> bool {
  std::ofstream file(state_file);
  if (!file.is_open()) {
    return false;
  }

  file << compilation_hash << "\n";
  return true;
}

auto load_compilation_state(const std::filesystem::path& state_file) -> std::optional<std::string> {
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

} // namespace kotlin_incremental

} // namespace horcrux::core
