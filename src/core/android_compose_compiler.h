// Horcrux - Android Jetpack Compose Compiler Support
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#ifndef HORCRUX_CORE_ANDROID_COMPOSE_COMPILER_H_
#define HORCRUX_CORE_ANDROID_COMPOSE_COMPILER_H_

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

namespace horcrux::core {

// Compose compiler plugin configuration
struct ComposeCompilerConfig {
  // Enable Compose compiler plugin
  bool enabled = false;

  // Compose compiler plugin version (e.g., "1.5.4")
  std::string version;

  // Compose compiler plugin JAR path
  std::filesystem::path plugin_jar;

  // Compose Kotlin version (e.g., "1.9.20")
  std::string kotlin_version;

  // Enable Compose metrics (compilation performance metrics)
  bool enable_metrics = false;

  // Output directory for Compose metrics
  std::filesystem::path metrics_output_dir;

  // Enable Compose reports (composable function reports)
  bool enable_reports = false;

  // Output directory for Compose reports
  std::filesystem::path reports_output_dir;

  // Enable live literals (experimental)
  bool enable_live_literals = false;

  // Enable source information (for better debugging)
  bool enable_source_information = true;

  // Enable intrinsic remember (optimization)
  bool enable_intrinsic_remember = true;

  // Suppress Kotlin version compatibility check
  bool suppress_kotlin_version_check = false;

  // Stability configuration file path (for cross-module stability)
  std::optional<std::filesystem::path> stability_config_path;

  // Additional Compose compiler options
  std::vector<std::string> additional_options;
};

// Compose metrics data
struct ComposeMetrics {
  // Total composable functions
  int total_composables = 0;

  // Restartable composables
  int restartable_composables = 0;

  // Skippable composables
  int skippable_composables = 0;

  // Readonly composables
  int readonly_composables = 0;

  // Total lambdas
  int total_lambdas = 0;

  // Singleton lambdas
  int singleton_lambdas = 0;

  // Total groups
  int total_groups = 0;

  // Compile time in milliseconds
  std::chrono::milliseconds compile_time{0};

  // Classes analyzed
  int classes_analyzed = 0;

  // Compose-specific hash for caching
  std::string compose_hash;
};

// Compose compilation result
struct ComposeCompilationResult {
  // Success status
  bool success = false;

  // Generated compose classes
  std::vector<std::filesystem::path> generated_classes;

  // Compose metrics
  std::optional<ComposeMetrics> metrics;

  // Compose compiler warnings
  std::vector<std::string> warnings;

  // Compose IR hash (for incremental compilation)
  std::string ir_hash;
};

// Helper functions for Compose compiler integration
namespace compose_compiler {

// Build Compose compiler plugin options for kotlinc
auto build_compose_plugin_options(const ComposeCompilerConfig& config) -> std::vector<std::string>;

// Validate Compose compiler configuration
auto validate_compose_config(const ComposeCompilerConfig& config)
    -> tl::expected<void, std::string>;

// Parse Compose metrics from output directory
auto parse_compose_metrics(const std::filesystem::path& metrics_dir)
    -> tl::expected<ComposeMetrics, std::string>;

// Compute Compose IR hash for caching
auto compute_compose_ir_hash(const std::vector<std::filesystem::path>& sources,
                             const ComposeCompilerConfig& config) -> std::string;

// Check if Compose compiler plugin is compatible with Kotlin version
auto is_compose_kotlin_compatible(const std::string& compose_version,
                                  const std::string& kotlin_version) -> bool;

// Scan generated Compose classes
auto scan_compose_generated_classes(const std::filesystem::path& output_dir)
    -> std::vector<std::filesystem::path>;

// Generate stability configuration file
auto generate_stability_config(const std::vector<std::string>& stable_types,
                               const std::filesystem::path& output_path)
    -> tl::expected<void, std::string>;

// Parse stability configuration file
auto parse_stability_config(const std::filesystem::path& config_path)
    -> tl::expected<std::vector<std::string>, std::string>;

} // namespace compose_compiler

} // namespace horcrux::core

#endif // HORCRUX_CORE_ANDROID_COMPOSE_COMPILER_H_
