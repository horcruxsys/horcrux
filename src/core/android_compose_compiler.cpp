// Horcrux - Android Jetpack Compose Compiler Support Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_compose_compiler.h"

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>

#include "local_cache.h" // For SHA-256 hashing

namespace horcrux::core {

namespace compose_compiler {

auto build_compose_plugin_options(const ComposeCompilerConfig& config) -> std::vector<std::string> {
  std::vector<std::string> options;

  if (!config.enabled) {
    return options;
  }

  // Add plugin JAR to classpath
  if (!config.plugin_jar.empty()) {
    options.push_back("-Xplugin=" + config.plugin_jar.string());
  }

  // Enable metrics
  if (config.enable_metrics && !config.metrics_output_dir.empty()) {
    options.push_back("-P");
    options.push_back("plugin:androidx.compose.compiler.plugins.kotlin:metricsDestination=" +
                     config.metrics_output_dir.string());
  }

  // Enable reports
  if (config.enable_reports && !config.reports_output_dir.empty()) {
    options.push_back("-P");
    options.push_back("plugin:androidx.compose.compiler.plugins.kotlin:reportsDestination=" +
                     config.reports_output_dir.string());
  }

  // Live literals (experimental)
  if (config.enable_live_literals) {
    options.push_back("-P");
    options.push_back("plugin:androidx.compose.compiler.plugins.kotlin:liveLiterals=true");
  } else {
    options.push_back("-P");
    options.push_back("plugin:androidx.compose.compiler.plugins.kotlin:liveLiterals=false");
  }

  // Source information
  if (config.enable_source_information) {
    options.push_back("-P");
    options.push_back("plugin:androidx.compose.compiler.plugins.kotlin:sourceInformation=true");
  }

  // Intrinsic remember
  if (config.enable_intrinsic_remember) {
    options.push_back("-P");
    options.push_back("plugin:androidx.compose.compiler.plugins.kotlin:intrinsicRemember=true");
  }

  // Suppress Kotlin version compatibility check
  if (config.suppress_kotlin_version_check) {
    options.push_back("-P");
    options.push_back(
        "plugin:androidx.compose.compiler.plugins.kotlin:suppressKotlinVersionCompatibilityCheck=true");
  }

  // Stability configuration
  if (config.stability_config_path) {
    options.push_back("-P");
    options.push_back("plugin:androidx.compose.compiler.plugins.kotlin:stabilityConfigurationPath=" +
                     config.stability_config_path->string());
  }

  // Additional options
  for (const auto& option : config.additional_options) {
    options.push_back(option);
  }

  return options;
}

auto validate_compose_config(const ComposeCompilerConfig& config)
    -> tl::expected<void, std::string> {
  if (!config.enabled) {
    return {}; // Nothing to validate if disabled
  }

  // Check plugin JAR exists
  if (config.plugin_jar.empty()) {
    return tl::unexpected("Compose compiler plugin JAR path is required");
  }

  if (!std::filesystem::exists(config.plugin_jar)) {
    return tl::unexpected("Compose compiler plugin JAR does not exist: " +
                         config.plugin_jar.string());
  }

  // Validate metrics directory
  if (config.enable_metrics) {
    if (config.metrics_output_dir.empty()) {
      return tl::unexpected("Metrics output directory is required when metrics are enabled");
    }
    // Create metrics directory if it doesn't exist
    std::error_code ec;
    std::filesystem::create_directories(config.metrics_output_dir, ec);
    if (ec) {
      return tl::unexpected("Failed to create metrics directory: " + ec.message());
    }
  }

  // Validate reports directory
  if (config.enable_reports) {
    if (config.reports_output_dir.empty()) {
      return tl::unexpected("Reports output directory is required when reports are enabled");
    }
    // Create reports directory if it doesn't exist
    std::error_code ec;
    std::filesystem::create_directories(config.reports_output_dir, ec);
    if (ec) {
      return tl::unexpected("Failed to create reports directory: " + ec.message());
    }
  }

  // Validate stability config if provided
  if (config.stability_config_path && !config.stability_config_path->empty()) {
    if (!std::filesystem::exists(*config.stability_config_path)) {
      return tl::unexpected("Stability configuration file does not exist: " +
                           config.stability_config_path->string());
    }
  }

  return {};
}

auto parse_compose_metrics(const std::filesystem::path& metrics_dir)
    -> tl::expected<ComposeMetrics, std::string> {
  ComposeMetrics metrics;

  // Look for metrics files (typically named *-module.json or *-composables.txt)
  if (!std::filesystem::exists(metrics_dir)) {
    return tl::unexpected("Metrics directory does not exist: " + metrics_dir.string());
  }

  // Parse composables count from metrics files
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(metrics_dir, ec)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    const auto& path = entry.path();
    std::string filename = path.filename().string();

    // Parse composables.txt files
    if (filename.find("composables") != std::string::npos && path.extension() == ".txt") {
      std::ifstream file(path);
      if (!file.is_open()) {
        continue;
      }

      std::string line;
      while (std::getline(file, line)) {
        // Count composables by parsing lines
        if (line.find("restartable") != std::string::npos) {
          metrics.restartable_composables++;
        }
        if (line.find("skippable") != std::string::npos) {
          metrics.skippable_composables++;
        }
        if (line.find("readonly") != std::string::npos) {
          metrics.readonly_composables++;
        }
        metrics.total_composables++;
      }
    }

    // Parse module.json files for detailed metrics
    if (filename.find("module") != std::string::npos && path.extension() == ".json") {
      // Simple JSON parsing for key metrics
      std::ifstream file(path);
      if (!file.is_open()) {
        continue;
      }

      std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

      // Extract metrics using regex (simplified parsing)
      std::regex classes_regex(R"("analyzedClasses":\s*(\d+))");
      std::smatch match;
      if (std::regex_search(content, match, classes_regex)) {
        metrics.classes_analyzed = std::stoi(match[1].str());
      }
    }
  }

  // Compute Compose hash from metrics
  std::ostringstream hash_input;
  hash_input << metrics.total_composables << ":" << metrics.restartable_composables << ":"
             << metrics.skippable_composables << ":" << metrics.readonly_composables;
  
  std::string hash_str = hash_input.str();
  std::vector<uint8_t> hash_data(hash_str.begin(), hash_str.end());
  auto hash = compute_sha256(hash_data);
  metrics.compose_hash = hash_to_string(hash);

  return metrics;
}

auto compute_compose_ir_hash(const std::vector<std::filesystem::path>& sources,
                             const ComposeCompilerConfig& config) -> std::string {
  // Compute hash for Compose IR caching
  std::ostringstream hash_input;

  // Add source files (sorted for determinism)
  std::vector<std::filesystem::path> sorted_sources = sources;
  std::sort(sorted_sources.begin(), sorted_sources.end());

  for (const auto& source : sorted_sources) {
    hash_input << source.string() << ":";
  }

  // Add Compose configuration
  hash_input << "compose:" << config.version << ":";
  hash_input << "kotlin:" << config.kotlin_version << ":";
  hash_input << "metrics:" << config.enable_metrics << ":";
  hash_input << "reports:" << config.enable_reports << ":";
  hash_input << "live_literals:" << config.enable_live_literals << ":";
  hash_input << "source_info:" << config.enable_source_information << ":";
  hash_input << "intrinsic_remember:" << config.enable_intrinsic_remember << ":";

  if (config.stability_config_path) {
    hash_input << "stability:" << config.stability_config_path->string() << ":";
  }

  std::string hash_str = hash_input.str();
  std::vector<uint8_t> hash_data(hash_str.begin(), hash_str.end());
  auto hash = compute_sha256(hash_data);
  return hash_to_string(hash);
}

auto is_compose_kotlin_compatible(const std::string& compose_version,
                                  const std::string& kotlin_version) -> bool {
  // Simplified compatibility check
  // In practice, this should check against a compatibility matrix

  // Compose 1.5.x requires Kotlin 1.9.x
  if (compose_version.starts_with("1.5.") && kotlin_version.starts_with("1.9.")) {
    return true;
  }

  // Compose 1.4.x requires Kotlin 1.8.x or 1.9.x
  if (compose_version.starts_with("1.4.")) {
    return kotlin_version.starts_with("1.8.") || kotlin_version.starts_with("1.9.");
  }

  // Compose 1.6.x requires Kotlin 1.9.x or 2.0.x
  if (compose_version.starts_with("1.6.")) {
    return kotlin_version.starts_with("1.9.") || kotlin_version.starts_with("2.0.");
  }

  // Default: allow if not explicitly incompatible
  return true;
}

auto scan_compose_generated_classes(const std::filesystem::path& output_dir)
    -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> generated_classes;

  if (!std::filesystem::exists(output_dir)) {
    return generated_classes;
  }

  std::error_code ec;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(output_dir, ec)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    const auto& path = entry.path();

    // Look for generated Compose classes (typically contain ComposableSingletons, ComposerKt, etc.)
    std::string filename = path.filename().string();
    if (filename.find("ComposableSingletons") != std::string::npos ||
        filename.find("ComposerKt") != std::string::npos || filename.find("Compose") != std::string::npos) {
      generated_classes.push_back(path);
    }
  }

  // Sort for determinism
  std::sort(generated_classes.begin(), generated_classes.end());

  return generated_classes;
}

auto generate_stability_config(const std::vector<std::string>& stable_types,
                               const std::filesystem::path& output_path)
    -> tl::expected<void, std::string> {
  std::ofstream config_file(output_path);
  if (!config_file.is_open()) {
    return tl::unexpected("Failed to create stability configuration file: " + output_path.string());
  }

  // Write stability configuration
  // Format: each stable type on a new line
  for (const auto& type : stable_types) {
    config_file << type << "\n";
  }

  config_file.close();

  if (config_file.fail()) {
    return tl::unexpected("Failed to write stability configuration file");
  }

  return {};
}

auto parse_stability_config(const std::filesystem::path& config_path)
    -> tl::expected<std::vector<std::string>, std::string> {
  if (!std::filesystem::exists(config_path)) {
    return tl::unexpected("Stability configuration file does not exist: " + config_path.string());
  }

  std::ifstream config_file(config_path);
  if (!config_file.is_open()) {
    return tl::unexpected("Failed to open stability configuration file: " + config_path.string());
  }

  std::vector<std::string> stable_types;
  std::string line;

  while (std::getline(config_file, line)) {
    // Skip empty lines and comments
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Trim whitespace
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" \t\r\n") + 1);

    if (!line.empty()) {
      stable_types.push_back(line);
    }
  }

  return stable_types;
}

} // namespace compose_compiler

} // namespace horcrux::core
