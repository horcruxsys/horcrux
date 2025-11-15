// Horcrux - Android D8/R8 DEX Compiler Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_dex_compiler.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>

#include "local_cache.h" // For SHA-256 hashing

namespace horcrux::core {

namespace {

// Execute a command and capture output
struct CommandResult {
  int exit_code;
  std::string stdout_output;
  std::string stderr_output;
};

auto execute_command(const std::vector<std::string>& command) -> CommandResult {
  CommandResult result{-1, "", ""};

  // Build command string
  std::ostringstream cmd_stream;
  for (size_t i = 0; i < command.size(); ++i) {
    if (i > 0) {
      cmd_stream << " ";
    }
    // Simple escaping - in production, use proper shell escaping
    if (command[i].find(' ') != std::string::npos) {
      cmd_stream << "\"" << command[i] << "\"";
    } else {
      cmd_stream << command[i];
    }
  }

  // Redirect stderr to stdout
  cmd_stream << " 2>&1";

  std::string cmd_str = cmd_stream.str();

  // Execute command
  std::array<char, 128> buffer{};
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd_str.c_str(), "r"), pclose);

  if (!pipe) {
    result.stderr_output = "Failed to execute command";
    return result;
  }

  // Read output
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
    result.stdout_output += buffer.data();
  }

  // Get exit code
  result.exit_code = pclose(pipe.release());

  return result;
}

// Get platform-specific classpath separator
auto get_classpath_separator() -> std::string {
#ifdef _WIN32
  return ";";
#else
  return ":";
#endif
}

// Build classpath string from paths
auto build_classpath_string(const std::vector<std::filesystem::path>& classpath) -> std::string {
  if (classpath.empty()) {
    return "";
  }

  std::ostringstream result;
  for (size_t i = 0; i < classpath.size(); ++i) {
    if (i > 0) {
      result << get_classpath_separator();
    }
    result << classpath[i].string();
  }
  return result.str();
}

} // anonymous namespace

auto to_string(DexCompilerError error) -> std::string {
  switch (error) {
  case DexCompilerError::CompilerNotFound:
    return "D8/R8 compiler not found in Android build tools";
  case DexCompilerError::InvalidInputFile:
    return "Invalid input file (must be .class or .jar)";
  case DexCompilerError::InvalidConfiguration:
    return "Invalid DEX compilation configuration";
  case DexCompilerError::CompilationFailed:
    return "DEX compilation failed";
  case DexCompilerError::ProguardConfigNotFound:
    return "ProGuard configuration file not found";
  case DexCompilerError::MergeFailed:
    return "DEX merge failed";
  case DexCompilerError::IoError:
    return "I/O error during DEX compilation";
  case DexCompilerError::UnknownError:
    return "Unknown error during DEX compilation";
  }
  return "Unknown error";
}

// AndroidDexCompiler implementation

AndroidDexCompiler::AndroidDexCompiler(const AndroidToolchain& toolchain)
    : toolchain_(toolchain) {}

auto AndroidDexCompiler::compile_d8(const D8CompileConfig& config)
    -> tl::expected<DexCompileResult, DexCompilerError> {
  // Validate configuration
  if (auto validation = validate_d8_config(config); !validation) {
    return tl::unexpected(validation.error());
  }

  // Validate D8 tool is available
  if (auto validation = validate_d8_tool(); !validation) {
    return tl::unexpected(validation.error());
  }

  // Check if incremental compilation is possible
  if (config.incremental) {
    auto state_file = config.output_dir / ".horcrux_d8_state";
    if (std::filesystem::exists(state_file)) {
      auto cached_state = dex_incremental::load_compilation_state(state_file);
      if (cached_state) {
        auto current_hash = compute_d8_hash(config);
        if (cached_state->first == current_hash) {
          // No compilation needed, return cached result
          DexCompileResult result;
          result.success = true;
          result.output_dir = config.output_dir;
          result.dex_files = AndroidDexCompiler::scan_dex_files(config.output_dir);
          result.compilation_hash = cached_state->second;
          result.compilation_time = std::chrono::milliseconds(0);
          return result;
        }
      }
    }
  }

  // Build D8 command
  auto command = build_d8_command(config);

  // Execute D8
  auto start_time = std::chrono::steady_clock::now();
  auto exec_result = execute_d8(command);
  auto end_time = std::chrono::steady_clock::now();

  if (!exec_result) {
    return tl::unexpected(exec_result.error());
  }

  auto& result = *exec_result;
  result.compilation_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Save compilation state
  if (config.incremental) {
    auto state_file = config.output_dir / ".horcrux_d8_state";
    auto config_hash = compute_d8_hash(config);
    dex_incremental::save_compilation_state(state_file, config_hash, result.compilation_hash);
  }

  return result;
}

auto AndroidDexCompiler::compile_r8(const R8CompileConfig& config)
    -> tl::expected<DexCompileResult, DexCompilerError> {
  // Validate configuration
  if (auto validation = validate_r8_config(config); !validation) {
    return tl::unexpected(validation.error());
  }

  // Validate R8 tool is available
  if (auto validation = validate_r8_tool(); !validation) {
    return tl::unexpected(validation.error());
  }

  // Check if incremental compilation is possible
  if (config.incremental) {
    auto state_file = config.output_dir / ".horcrux_r8_state";
    if (std::filesystem::exists(state_file)) {
      auto cached_state = dex_incremental::load_compilation_state(state_file);
      if (cached_state) {
        auto current_hash = compute_r8_hash(config);
        if (cached_state->first == current_hash) {
          // No compilation needed, return cached result
          DexCompileResult result;
          result.success = true;
          result.output_dir = config.output_dir;
          result.dex_files = AndroidDexCompiler::scan_dex_files(config.output_dir);
          result.compilation_hash = cached_state->second;
          result.compilation_time = std::chrono::milliseconds(0);
          if (config.proguard_config.mapping_output &&
              std::filesystem::exists(*config.proguard_config.mapping_output)) {
            result.mapping_file = config.proguard_config.mapping_output;
          }
          return result;
        }
      }
    }
  }

  // Build R8 command
  auto command = build_r8_command(config);

  // Execute R8
  auto start_time = std::chrono::steady_clock::now();
  auto exec_result = execute_r8(command);
  auto end_time = std::chrono::steady_clock::now();

  if (!exec_result) {
    return tl::unexpected(exec_result.error());
  }

  auto& result = *exec_result;
  result.compilation_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Add mapping file if it was generated
  if (config.proguard_config.mapping_output &&
      std::filesystem::exists(*config.proguard_config.mapping_output)) {
    result.mapping_file = config.proguard_config.mapping_output;
  }

  // Save compilation state
  if (config.incremental) {
    auto state_file = config.output_dir / ".horcrux_r8_state";
    auto config_hash = compute_r8_hash(config);
    dex_incremental::save_compilation_state(state_file, config_hash, result.compilation_hash);
  }

  return result;
}

auto AndroidDexCompiler::merge_dex(const DexMergeConfig& config)
    -> tl::expected<DexCompileResult, DexCompilerError> {
  if (config.dex_files.empty()) {
    return tl::unexpected(DexCompilerError::InvalidConfiguration);
  }

  // Validate all input files exist
  for (const auto& dex_file : config.dex_files) {
    if (!std::filesystem::exists(dex_file)) {
      return tl::unexpected(DexCompilerError::InvalidInputFile);
    }
  }

  // Merge using D8
  bool success = dex_merge::merge_dex_files(config.dex_files, config.output_file, config.min_api);

  if (!success) {
    return tl::unexpected(DexCompilerError::MergeFailed);
  }

  DexCompileResult result;
  result.success = true;
  result.output_dir = config.output_file.parent_path();
  result.dex_files = {config.output_file};
  result.compilation_hash = dex_incremental::compute_input_hash(config.dex_files);
  result.compilation_time = std::chrono::milliseconds(0);

  return result;
}

auto AndroidDexCompiler::validate_d8_config(const D8CompileConfig& config)
    -> tl::expected<void, DexCompilerError> {
  if (config.inputs.empty()) {
    return tl::unexpected(DexCompilerError::InvalidConfiguration);
  }

  // Validate input files exist
  for (const auto& input : config.inputs) {
    if (!std::filesystem::exists(input)) {
      return tl::unexpected(DexCompilerError::InvalidInputFile);
    }
  }

  // Validate output directory
  if (config.output_dir.empty()) {
    return tl::unexpected(DexCompilerError::InvalidConfiguration);
  }

  // Validate min API
  if (config.min_api < 1 || config.min_api > 100) {
    return tl::unexpected(DexCompilerError::InvalidConfiguration);
  }

  // Validate main dex list if specified
  if (config.main_dex_list && !std::filesystem::exists(*config.main_dex_list)) {
    return tl::unexpected(DexCompilerError::InvalidConfiguration);
  }

  return {};
}

auto AndroidDexCompiler::validate_r8_config(const R8CompileConfig& config)
    -> tl::expected<void, DexCompilerError> {
  if (config.inputs.empty()) {
    return tl::unexpected(DexCompilerError::InvalidConfiguration);
  }

  // Validate input files exist
  for (const auto& input : config.inputs) {
    if (!std::filesystem::exists(input)) {
      return tl::unexpected(DexCompilerError::InvalidInputFile);
    }
  }

  // Validate output directory
  if (config.output_dir.empty()) {
    return tl::unexpected(DexCompilerError::InvalidConfiguration);
  }

  // Validate min API
  if (config.min_api < 1 || config.min_api > 100) {
    return tl::unexpected(DexCompilerError::InvalidConfiguration);
  }

  // Validate ProGuard config files
  for (const auto& pg_file : config.proguard_config.config_files) {
    if (!std::filesystem::exists(pg_file)) {
      return tl::unexpected(DexCompilerError::ProguardConfigNotFound);
    }
  }

  // Validate main dex list if specified
  if (config.main_dex_list && !std::filesystem::exists(*config.main_dex_list)) {
    return tl::unexpected(DexCompilerError::InvalidConfiguration);
  }

  return {};
}

auto AndroidDexCompiler::compute_d8_hash(const D8CompileConfig& config) -> std::string {
  std::ostringstream hash_input;

  // Hash inputs (sorted for determinism)
  std::vector<std::filesystem::path> sorted_inputs = config.inputs;
  std::sort(sorted_inputs.begin(), sorted_inputs.end());
  for (const auto& input : sorted_inputs) {
    hash_input << input.string() << "|";
  }

  // Hash classpath (sorted)
  std::vector<std::filesystem::path> sorted_classpath = config.classpath;
  std::sort(sorted_classpath.begin(), sorted_classpath.end());
  for (const auto& cp : sorted_classpath) {
    hash_input << cp.string() << "|";
  }

  // Hash configuration options
  hash_input << config.min_api << "|";
  hash_input << config.multi_dex << "|";
  hash_input << config.debug << "|";

  if (config.main_dex_list) {
    hash_input << config.main_dex_list->string() << "|";
  }

  // Hash additional options (sorted)
  std::vector<std::string> sorted_options = config.d8_options;
  std::sort(sorted_options.begin(), sorted_options.end());
  for (const auto& opt : sorted_options) {
    hash_input << opt << "|";
  }

  // Compute SHA-256 hash of the input string
  std::string input_str = hash_input.str();
  std::vector<uint8_t> input_bytes(input_str.begin(), input_str.end());
  auto hash = compute_sha256(input_bytes);
  return hash_to_string(hash);
}

auto AndroidDexCompiler::compute_r8_hash(const R8CompileConfig& config) -> std::string {
  std::ostringstream hash_input;

  // Hash inputs (sorted for determinism)
  std::vector<std::filesystem::path> sorted_inputs = config.inputs;
  std::sort(sorted_inputs.begin(), sorted_inputs.end());
  for (const auto& input : sorted_inputs) {
    hash_input << input.string() << "|";
  }

  // Hash classpath (sorted)
  std::vector<std::filesystem::path> sorted_classpath = config.classpath;
  std::sort(sorted_classpath.begin(), sorted_classpath.end());
  for (const auto& cp : sorted_classpath) {
    hash_input << cp.string() << "|";
  }

  // Hash configuration options
  hash_input << config.min_api << "|";
  hash_input << config.multi_dex << "|";
  hash_input << config.debug << "|";

  // Hash ProGuard config
  hash_input << config.proguard_config.optimize << "|";
  hash_input << config.proguard_config.obfuscate << "|";
  hash_input << config.proguard_config.shrink << "|";

  std::vector<std::filesystem::path> sorted_pg_files = config.proguard_config.config_files;
  std::sort(sorted_pg_files.begin(), sorted_pg_files.end());
  for (const auto& pg_file : sorted_pg_files) {
    hash_input << pg_file.string() << "|";
  }

  if (config.main_dex_list) {
    hash_input << config.main_dex_list->string() << "|";
  }

  // Hash additional options (sorted)
  std::vector<std::string> sorted_options = config.r8_options;
  std::sort(sorted_options.begin(), sorted_options.end());
  for (const auto& opt : sorted_options) {
    hash_input << opt << "|";
  }

  // Compute SHA-256 hash of the input string
  std::string input_str = hash_input.str();
  std::vector<uint8_t> input_bytes(input_str.begin(), input_str.end());
  auto hash = compute_sha256(input_bytes);
  return hash_to_string(hash);
}

  // Hash inputs (sorted for determinism)
  std::vector<std::filesystem::path> sorted_inputs = config.inputs;
  std::sort(sorted_inputs.begin(), sorted_inputs.end());
  for (const auto& input : sorted_inputs) {
    hash_input << input.string() << "|";
  }

  // Hash classpath (sorted)
  std::vector<std::filesystem::path> sorted_classpath = config.classpath;
  std::sort(sorted_classpath.begin(), sorted_classpath.end());
  for (const auto& cp : sorted_classpath) {
    hash_input << cp.string() << "|";
  }

  // Hash configuration options
  hash_input << config.min_api << "|";
  hash_input << config.multi_dex << "|";
  hash_input << config.debug << "|";

  // Hash ProGuard config
  hash_input << config.proguard_config.optimize << "|";
  hash_input << config.proguard_config.obfuscate << "|";
  hash_input << config.proguard_config.shrink << "|";

  std::vector<std::filesystem::path> sorted_pg_files = config.proguard_config.config_files;
  std::sort(sorted_pg_files.begin(), sorted_pg_files.end());
  for (const auto& pg_file : sorted_pg_files) {
    hash_input << pg_file.string() << "|";
  }

  if (config.main_dex_list) {
    hash_input << config.main_dex_list->string() << "|";
  }

  // Hash additional options (sorted)
  std::vector<std::string> sorted_options = config.r8_options;
  std::sort(sorted_options.begin(), sorted_options.end());
  for (const auto& opt : sorted_options) {
    hash_input << opt << "|";
  }

  return cache.compute_hash(hash_input.str());
}

auto AndroidDexCompiler::is_d8_compilation_needed(const D8CompileConfig& config,
                                                   const std::string& cached_hash) const -> bool {
  auto current_hash = compute_d8_hash(config);
  return current_hash != cached_hash;
}

auto AndroidDexCompiler::is_r8_compilation_needed(const R8CompileConfig& config,
                                                   const std::string& cached_hash) const -> bool {
  auto current_hash = compute_r8_hash(config);
  return current_hash != cached_hash;
}

auto AndroidDexCompiler::parse_proguard_config(const std::filesystem::path& config_path)
    -> tl::expected<ProguardConfig, DexCompilerError> {
  if (!std::filesystem::exists(config_path)) {
    return tl::unexpected(DexCompilerError::ProguardConfigNotFound);
  }

  ProguardConfig config;
  config.config_files.push_back(config_path);

  // Load rules from file
  auto rules_result = proguard_utils::load_proguard_rules(config_path);
  if (!rules_result) {
    return tl::unexpected(rules_result.error());
  }

  config.keep_rules = *rules_result;

  return config;
}

auto AndroidDexCompiler::build_d8_command(const D8CompileConfig& config) const
    -> std::vector<std::string> {
  std::vector<std::string> command;

  // Get D8 path
  auto d8_path = get_d8_path();
  if (!d8_path) {
    return command;
  }

  command.push_back(d8_path->string());

  // Add output directory
  command.push_back("--output");
  command.push_back(config.output_dir.string());

  // Add min API
  command.push_back("--min-api");
  command.push_back(std::to_string(config.min_api));

  // Add classpath if not empty
  if (!config.classpath.empty()) {
    command.push_back("--classpath");
    command.push_back(build_classpath_string(config.classpath));
  }

  // Add debug flag
  if (config.debug) {
    command.push_back("--debug");
  } else {
    command.push_back("--release");
  }

  // Add multi-dex flag
  if (config.multi_dex) {
    // D8 handles multi-dex automatically when needed
  }

  // Add main dex list if specified
  if (config.main_dex_list) {
    command.push_back("--main-dex-list");
    command.push_back(config.main_dex_list->string());
  }

  // Add additional options
  for (const auto& opt : config.d8_options) {
    command.push_back(opt);
  }

  // Add input files (sorted for determinism)
  std::vector<std::filesystem::path> sorted_inputs = config.inputs;
  std::sort(sorted_inputs.begin(), sorted_inputs.end());
  for (const auto& input : sorted_inputs) {
    command.push_back(input.string());
  }

  return command;
}

auto AndroidDexCompiler::build_r8_command(const R8CompileConfig& config) const
    -> std::vector<std::string> {
  std::vector<std::string> command;

  // Get R8 path
  auto r8_path = get_r8_path();
  if (!r8_path) {
    return command;
  }

  command.push_back(r8_path->string());

  // Add output directory
  command.push_back("--output");
  command.push_back(config.output_dir.string());

  // Add min API
  command.push_back("--min-api");
  command.push_back(std::to_string(config.min_api));

  // Add classpath if not empty
  if (!config.classpath.empty()) {
    command.push_back("--classpath");
    command.push_back(build_classpath_string(config.classpath));
  }

  // Add debug flag
  if (config.debug) {
    command.push_back("--debug");
  } else {
    command.push_back("--release");
  }

  // Add ProGuard configuration files
  for (const auto& pg_file : config.proguard_config.config_files) {
    command.push_back("--pg-conf");
    command.push_back(pg_file.string());
  }

  // Add ProGuard keep rules
  for (const auto& keep_rule : config.proguard_config.keep_rules) {
    command.push_back("--pg-conf-rule");
    command.push_back(keep_rule);
  }

  // Add optimization flags
  if (!config.proguard_config.optimize) {
    command.push_back("--no-optimize");
  }

  if (!config.proguard_config.obfuscate) {
    command.push_back("--no-obfuscate");
  }

  if (!config.proguard_config.shrink) {
    command.push_back("--no-shrink");
  }

  // Add mapping output
  if (config.proguard_config.mapping_output) {
    command.push_back("--pg-map-output");
    command.push_back(config.proguard_config.mapping_output->string());
  }

  // Add main dex list if specified
  if (config.main_dex_list) {
    command.push_back("--main-dex-list");
    command.push_back(config.main_dex_list->string());
  }

  // Add additional options
  for (const auto& opt : config.r8_options) {
    command.push_back(opt);
  }

  // Add input files (sorted for determinism)
  std::vector<std::filesystem::path> sorted_inputs = config.inputs;
  std::sort(sorted_inputs.begin(), sorted_inputs.end());
  for (const auto& input : sorted_inputs) {
    command.push_back(input.string());
  }

  return command;
}

auto AndroidDexCompiler::execute_d8(const std::vector<std::string>& command) const
    -> tl::expected<DexCompileResult, DexCompilerError> {
  // Create output directory
  std::filesystem::create_directories(command[2]); // output_dir is at index 2

  // Execute command
  auto cmd_result = execute_command(command);

  DexCompileResult result;
  result.stdout_output = cmd_result.stdout_output;
  result.stderr_output = cmd_result.stderr_output;

  if (cmd_result.exit_code != 0) {
    result.success = false;
    return tl::unexpected(DexCompilerError::CompilationFailed);
  }

  result.success = true;
  result.output_dir = command[2]; // output_dir
  result.dex_files = AndroidDexCompiler::scan_dex_files(result.output_dir);

  // Sort DEX files deterministically
  AndroidDexCompiler::sort_dex_files_deterministic(result.dex_files);

  // Compute output hash
  result.compilation_hash = dex_incremental::compute_input_hash(result.dex_files);

  return result;
}

auto AndroidDexCompiler::execute_r8(const std::vector<std::string>& command) const
    -> tl::expected<DexCompileResult, DexCompilerError> {
  // Create output directory
  std::filesystem::create_directories(command[2]); // output_dir is at index 2

  // Execute command
  auto cmd_result = execute_command(command);

  DexCompileResult result;
  result.stdout_output = cmd_result.stdout_output;
  result.stderr_output = cmd_result.stderr_output;

  if (cmd_result.exit_code != 0) {
    result.success = false;
    return tl::unexpected(DexCompilerError::CompilationFailed);
  }

  result.success = true;
  result.output_dir = command[2]; // output_dir
  result.dex_files = AndroidDexCompiler::scan_dex_files(result.output_dir);

  // Sort DEX files deterministically
  AndroidDexCompiler::sort_dex_files_deterministic(result.dex_files);

  // Compute output hash
  result.compilation_hash = dex_incremental::compute_input_hash(result.dex_files);

  // Try to extract shrunk classes count from output
  std::regex shrunk_regex(R"(Removed\s+(\d+)\s+classes)");
  std::smatch match;
  if (std::regex_search(result.stdout_output, match, shrunk_regex)) {
    result.shrunk_classes_count = std::stoull(match[1].str());
  }

  return result;
}

auto AndroidDexCompiler::scan_dex_files(const std::filesystem::path& output_dir)
    -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> dex_files;

  if (!std::filesystem::exists(output_dir)) {
    return dex_files;
  }

  try {
    for (const auto& entry : std::filesystem::directory_iterator(output_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".dex") {
        dex_files.push_back(entry.path());
      }
    }
  } catch (const std::filesystem::filesystem_error&) {
    // Ignore errors
  }

  return dex_files;
}

auto AndroidDexCompiler::get_d8_path() const -> std::optional<std::filesystem::path> {
  if (toolchain_.build_tools.empty()) {
    return std::nullopt;
  }

  // Use the first (newest) build tools
  const auto& bt = toolchain_.build_tools[0];
  if (std::filesystem::exists(bt.d8_path)) {
    return bt.d8_path;
  }

  return std::nullopt;
}

auto AndroidDexCompiler::get_r8_path() const -> std::optional<std::filesystem::path> {
  if (toolchain_.build_tools.empty()) {
    return std::nullopt;
  }
auto AndroidDexCompiler::validate_d8_tool() const -> tl::expected<void, DexCompilerError> {
  auto d8_path = get_d8_path();
  if (!d8_path || !std::filesystem::exists(*d8_path)) {
    return tl::unexpected(DexCompilerError::CompilerNotFound);
  }
  return {};
}

auto AndroidDexCompiler::validate_r8_tool() const -> tl::expected<void, DexCompilerError> {
  auto r8_path = get_r8_path();
  if (!r8_path || !std::filesystem::exists(*r8_path)) {
    return tl::unexpected(DexCompilerError::CompilerNotFound);
  }
  return {};
}

auto AndroidDexCompiler::sort_dex_files_deterministic(std::vector<std::filesystem::path>& dex_files)
    -> void {
  // Sort by filename: classes.dex, classes2.dex, classes3.dex, ...
  std::sort(dex_files.begin(), dex_files.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
              std::string a_name = a.filename().string();
              std::string b_name = b.filename().string();

              // Extract number from classesN.dex
              std::regex num_regex(R"(classes(\d*).dex)");
              std::smatch a_match;
              std::smatch b_match;

              int a_num = 1;
              int b_num = 1;

              if (std::regex_match(a_name, a_match, num_regex)) {
                if (a_match[1].length() > 0) {
                  a_num = std::stoi(a_match[1].str());
                }
              }

              if (std::regex_match(b_name, b_match, num_regex)) {
                if (b_match[1].length() > 0) {
                  b_num = std::stoi(b_match[1].str());
                }
              }

              return a_num < b_num;
            });
}


  // Use the first (newest) build tools
  const auto& bt = toolchain_.build_tools[0];
  if (std::filesystem::exists(bt.r8_path)) {
    return bt.r8_path;
  }

  return std::nullopt;
}

// Helper namespace implementations

namespace dex_merge {

auto merge_dex_files(const std::vector<std::filesystem::path>& dex_files,
                     const std::filesystem::path& output_file, int min_api) -> bool {
  // This is a simplified implementation
  // In production, you would use D8 to merge DEX files
  (void)dex_files;
  (void)output_file;
  (void)min_api;
  return false; // Not implemented yet
}

auto split_dex(const std::filesystem::path& input_dex,
               const std::filesystem::path& output_dir,
               const std::optional<std::filesystem::path>& main_dex_list) -> bool {
  (void)input_dex;
  (void)output_dir;
  (void)main_dex_list;
  return false; // Not implemented yet
}

auto get_method_count(const std::filesystem::path& dex_file) -> size_t {
  (void)dex_file;
  return 0; // Not implemented yet - would need to parse DEX file
}

auto needs_multi_dex(const std::filesystem::path& dex_file) -> bool {
  return get_method_count(dex_file) > 65536;
}

} // namespace dex_merge

namespace dex_incremental {

using horcrux::core::compute_sha256;
using horcrux::core::hash_to_string;

auto compute_input_hash(const std::vector<std::filesystem::path>& inputs) -> std::string {
  std::ostringstream hash_input;

  std::vector<std::filesystem::path> sorted_inputs = inputs;
  std::sort(sorted_inputs.begin(), sorted_inputs.end());

  for (const auto& input : sorted_inputs) {
    // Hash file path and modification time
    hash_input << input.string() << "|";
    if (std::filesystem::exists(input)) {
      auto last_write = std::filesystem::last_write_time(input);
      hash_input << last_write.time_since_epoch().count() << "|";
    }
  }

  // Compute SHA-256 hash of the input string
  std::string input_str = hash_input.str();
  std::vector<uint8_t> input_bytes(input_str.begin(), input_str.end());
  auto hash = compute_sha256(input_bytes);
  return hash_to_string(hash);
}

auto has_input_changed(const std::vector<std::filesystem::path>& inputs,
                       const std::string& cached_hash) -> bool {
  auto current_hash = compute_input_hash(inputs);
  return current_hash != cached_hash;
}

auto save_compilation_state(const std::filesystem::path& state_file,
                            const std::string& config_hash,
                            const std::string& compilation_hash) -> bool {
  try {
    std::ofstream file(state_file);
    if (!file.is_open()) {
      return false;
    }

    file << config_hash << "\n";
    file << compilation_hash << "\n";

    return true;
  } catch (...) {
    return false;
  }
}

auto load_compilation_state(const std::filesystem::path& state_file)
    -> std::optional<std::pair<std::string, std::string>> {
  try {
    std::ifstream file(state_file);
    if (!file.is_open()) {
      return std::nullopt;
    }

    std::string config_hash;
    std::string compilation_hash;

    if (!std::getline(file, config_hash) || !std::getline(file, compilation_hash)) {
      return std::nullopt;
    }

    return std::make_pair(config_hash, compilation_hash);
  } catch (...) {
    return std::nullopt;
  }
}

} // namespace dex_incremental

namespace proguard_utils {

using horcrux::core::ProguardConfig;
using horcrux::core::DexCompilerError;

auto load_proguard_rules(const std::filesystem::path& config_path)
    -> tl::expected<std::vector<std::string>, DexCompilerError> {
  std::vector<std::string> rules;

  try {
    std::ifstream file(config_path);
    if (!file.is_open()) {
      return tl::unexpected(DexCompilerError::ProguardConfigNotFound);
    }

    std::string line;
    while (std::getline(file, line)) {
      // Trim whitespace
      line.erase(0, line.find_first_not_of(" \t"));
      line.erase(line.find_last_not_of(" \t") + 1);

      // Skip empty lines and comments
      if (line.empty() || line[0] == '#') {
        continue;
      }

      rules.push_back(line);
    }

    return rules;
  } catch (...) {
    return tl::unexpected(DexCompilerError::IoError);
  }
}

auto generate_default_keep_rules() -> std::vector<std::string> {
  return {
      "-keep public class * extends android.app.Activity",
      "-keep public class * extends android.app.Service",
      "-keep public class * extends android.content.BroadcastReceiver",
      "-keep public class * extends android.content.ContentProvider",
      "-keepclassmembers class * implements android.os.Parcelable {",
      "  public static final ** CREATOR;",
      "}",
  };
}

auto validate_proguard_config(const ProguardConfig& config) -> bool {
  // Check that all config files exist
  for (const auto& config_file : config.config_files) {
    if (!std::filesystem::exists(config_file)) {
      return false;
    }
  }
  return true;
}

auto merge_proguard_configs(const std::vector<ProguardConfig>& configs)
    -> ProguardConfig {
  ProguardConfig merged;

  for (const auto& config : configs) {
    // Merge config files
    merged.config_files.insert(merged.config_files.end(), config.config_files.begin(),
                               config.config_files.end());

    // Merge keep rules
    merged.keep_rules.insert(merged.keep_rules.end(), config.keep_rules.begin(),
                             config.keep_rules.end());

    // Merge keep attributes
    merged.keep_attributes.insert(merged.keep_attributes.end(), config.keep_attributes.begin(),
                                  config.keep_attributes.end());

    // Use AND logic for optimization flags
    merged.optimize = merged.optimize && config.optimize;
    merged.obfuscate = merged.obfuscate && config.obfuscate;
    merged.shrink = merged.shrink && config.shrink;
  }

  return merged;
}


} // namespace horcrux::core
