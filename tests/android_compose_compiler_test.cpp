// Horcrux - Android Jetpack Compose Compiler Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/core/android_compose_compiler.h"
#include "../src/core/android_kotlin_compiler.h"

namespace horcrux::core::test {

// Helper to create a temporary directory for testing
class TempDir {
public:
  TempDir() {
    path_ = std::filesystem::temp_directory_path() / "horcrux_compose_test";
    std::filesystem::remove_all(path_); // Clean up if exists
    std::filesystem::create_directories(path_);
  }

  ~TempDir() {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }

  auto path() const -> const std::filesystem::path& {
    return path_;
  }

private:
  std::filesystem::path path_;
};

// Test Compose configuration validation
TEST(ComposeCompilerTest, ValidateConfigRequiresPluginJar) {
  ComposeCompilerConfig config;
  config.enabled = true;
  // Missing plugin_jar should fail validation

  auto result = compose_compiler::validate_compose_config(config);
  EXPECT_FALSE(result.has_value());
}

TEST(ComposeCompilerTest, ValidateConfigAcceptsValidConfig) {
  TempDir temp;

  ComposeCompilerConfig config;
  config.enabled = true;

  // Create mock plugin JAR
  auto plugin_jar = temp.path() / "compose-compiler.jar";
  std::ofstream(plugin_jar).close();
  config.plugin_jar = plugin_jar;

  auto result = compose_compiler::validate_compose_config(config);
  EXPECT_TRUE(result.has_value());
}

TEST(ComposeCompilerTest, ValidateConfigCreatesMetricsDirectory) {
  TempDir temp;

  ComposeCompilerConfig config;
  config.enabled = true;

  auto plugin_jar = temp.path() / "compose-compiler.jar";
  std::ofstream(plugin_jar).close();
  config.plugin_jar = plugin_jar;

  config.enable_metrics = true;
  config.metrics_output_dir = temp.path() / "metrics";

  auto result = compose_compiler::validate_compose_config(config);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(std::filesystem::exists(config.metrics_output_dir));
}

TEST(ComposeCompilerTest, ValidateConfigCreatesReportsDirectory) {
  TempDir temp;

  ComposeCompilerConfig config;
  config.enabled = true;

  auto plugin_jar = temp.path() / "compose-compiler.jar";
  std::ofstream(plugin_jar).close();
  config.plugin_jar = plugin_jar;

  config.enable_reports = true;
  config.reports_output_dir = temp.path() / "reports";

  auto result = compose_compiler::validate_compose_config(config);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(std::filesystem::exists(config.reports_output_dir));
}

TEST(ComposeCompilerTest, ValidateConfigChecksStabilityConfigExists) {
  TempDir temp;

  ComposeCompilerConfig config;
  config.enabled = true;

  auto plugin_jar = temp.path() / "compose-compiler.jar";
  std::ofstream(plugin_jar).close();
  config.plugin_jar = plugin_jar;

  // Non-existent stability config should fail
  config.stability_config_path = temp.path() / "nonexistent.txt";

  auto result = compose_compiler::validate_compose_config(config);
  EXPECT_FALSE(result.has_value());
}

// Test plugin options generation
TEST(ComposeCompilerTest, BuildPluginOptionsIncludesPluginJar) {
  TempDir temp;

  ComposeCompilerConfig config;
  config.enabled = true;
  config.plugin_jar = temp.path() / "compose-compiler.jar";

  auto options = compose_compiler::build_compose_plugin_options(config);

  EXPECT_FALSE(options.empty());
  EXPECT_TRUE(std::find_if(options.begin(), options.end(), [](const std::string& opt) {
                return opt.find("-Xplugin=") != std::string::npos;
              }) != options.end());
}

TEST(ComposeCompilerTest, BuildPluginOptionsIncludesMetrics) {
  TempDir temp;

  ComposeCompilerConfig config;
  config.enabled = true;
  config.plugin_jar = temp.path() / "compose-compiler.jar";
  config.enable_metrics = true;
  config.metrics_output_dir = temp.path() / "metrics";

  auto options = compose_compiler::build_compose_plugin_options(config);

  EXPECT_TRUE(std::find_if(options.begin(), options.end(), [](const std::string& opt) {
                return opt.find("metricsDestination=") != std::string::npos;
              }) != options.end());
}

TEST(ComposeCompilerTest, BuildPluginOptionsIncludesReports) {
  TempDir temp;

  ComposeCompilerConfig config;
  config.enabled = true;
  config.plugin_jar = temp.path() / "compose-compiler.jar";
  config.enable_reports = true;
  config.reports_output_dir = temp.path() / "reports";

  auto options = compose_compiler::build_compose_plugin_options(config);

  EXPECT_TRUE(std::find_if(options.begin(), options.end(), [](const std::string& opt) {
                return opt.find("reportsDestination=") != std::string::npos;
              }) != options.end());
}

TEST(ComposeCompilerTest, BuildPluginOptionsIncludesLiveLiterals) {
  ComposeCompilerConfig config;
  config.enabled = true;
  config.plugin_jar = "/path/to/compose-compiler.jar";
  config.enable_live_literals = true;

  auto options = compose_compiler::build_compose_plugin_options(config);

  EXPECT_TRUE(std::find_if(options.begin(), options.end(), [](const std::string& opt) {
                return opt.find("liveLiterals=true") != std::string::npos;
              }) != options.end());
}

TEST(ComposeCompilerTest, BuildPluginOptionsIncludesSourceInformation) {
  ComposeCompilerConfig config;
  config.enabled = true;
  config.plugin_jar = "/path/to/compose-compiler.jar";
  config.enable_source_information = true;

  auto options = compose_compiler::build_compose_plugin_options(config);

  EXPECT_TRUE(std::find_if(options.begin(), options.end(), [](const std::string& opt) {
                return opt.find("sourceInformation=true") != std::string::npos;
              }) != options.end());
}

TEST(ComposeCompilerTest, BuildPluginOptionsIncludesIntrinsicRemember) {
  ComposeCompilerConfig config;
  config.enabled = true;
  config.plugin_jar = "/path/to/compose-compiler.jar";
  config.enable_intrinsic_remember = true;

  auto options = compose_compiler::build_compose_plugin_options(config);

  EXPECT_TRUE(std::find_if(options.begin(), options.end(), [](const std::string& opt) {
                return opt.find("intrinsicRemember=true") != std::string::npos;
              }) != options.end());
}

TEST(ComposeCompilerTest, BuildPluginOptionsIncludesStabilityConfig) {
  TempDir temp;
  auto stability_path = temp.path() / "stability.txt";

  ComposeCompilerConfig config;
  config.enabled = true;
  config.plugin_jar = "/path/to/compose-compiler.jar";
  config.stability_config_path = stability_path;

  auto options = compose_compiler::build_compose_plugin_options(config);

  EXPECT_TRUE(std::find_if(options.begin(), options.end(), [&](const std::string& opt) {
                return opt.find("stabilityConfigurationPath=") != std::string::npos;
              }) != options.end());
}

// Test IR hash computation
TEST(ComposeCompilerTest, ComputeIRHashIsDeterministic) {
  std::vector<std::filesystem::path> sources = {"/path/to/MainActivity.kt", "/path/to/Theme.kt"};

  ComposeCompilerConfig config;
  config.enabled = true;
  config.version = "1.5.4";
  config.kotlin_version = "1.9.20";

  std::string hash1 = compose_compiler::compute_compose_ir_hash(sources, config);
  std::string hash2 = compose_compiler::compute_compose_ir_hash(sources, config);

  EXPECT_EQ(hash1, hash2);
  EXPECT_FALSE(hash1.empty());
}

TEST(ComposeCompilerTest, ComputeIRHashChangesWithDifferentSources) {
  std::vector<std::filesystem::path> sources1 = {"/path/to/MainActivity.kt"};
  std::vector<std::filesystem::path> sources2 = {"/path/to/Theme.kt"};

  ComposeCompilerConfig config;
  config.enabled = true;
  config.version = "1.5.4";
  config.kotlin_version = "1.9.20";

  std::string hash1 = compose_compiler::compute_compose_ir_hash(sources1, config);
  std::string hash2 = compose_compiler::compute_compose_ir_hash(sources2, config);

  EXPECT_NE(hash1, hash2);
}

TEST(ComposeCompilerTest, ComputeIRHashChangesWithDifferentConfig) {
  std::vector<std::filesystem::path> sources = {"/path/to/MainActivity.kt"};

  ComposeCompilerConfig config1;
  config1.enabled = true;
  config1.version = "1.5.4";
  config1.kotlin_version = "1.9.20";
  config1.enable_metrics = false;

  ComposeCompilerConfig config2;
  config2.enabled = true;
  config2.version = "1.5.4";
  config2.kotlin_version = "1.9.20";
  config2.enable_metrics = true;

  std::string hash1 = compose_compiler::compute_compose_ir_hash(sources, config1);
  std::string hash2 = compose_compiler::compute_compose_ir_hash(sources, config2);

  EXPECT_NE(hash1, hash2);
}

// Test Kotlin version compatibility
TEST(ComposeCompilerTest, KotlinCompatibilityCheck_1_5_With_1_9) {
  EXPECT_TRUE(compose_compiler::is_compose_kotlin_compatible("1.5.4", "1.9.20"));
}

TEST(ComposeCompilerTest, KotlinCompatibilityCheck_1_4_With_1_8) {
  EXPECT_TRUE(compose_compiler::is_compose_kotlin_compatible("1.4.8", "1.8.10"));
}

TEST(ComposeCompilerTest, KotlinCompatibilityCheck_1_4_With_1_9) {
  EXPECT_TRUE(compose_compiler::is_compose_kotlin_compatible("1.4.8", "1.9.0"));
}

TEST(ComposeCompilerTest, KotlinCompatibilityCheck_1_6_With_1_9) {
  EXPECT_TRUE(compose_compiler::is_compose_kotlin_compatible("1.6.0", "1.9.20"));
}

TEST(ComposeCompilerTest, KotlinCompatibilityCheck_1_6_With_2_0) {
  EXPECT_TRUE(compose_compiler::is_compose_kotlin_compatible("1.6.0", "2.0.0"));
}

// Test stability configuration
TEST(ComposeCompilerTest, GenerateStabilityConfig) {
  TempDir temp;
  auto config_path = temp.path() / "stability.txt";

  std::vector<std::string> stable_types = {"com.example.MyStableClass",
                                           "com.example.AnotherStableClass"};

  auto result = compose_compiler::generate_stability_config(stable_types, config_path);

  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(std::filesystem::exists(config_path));

  // Verify content
  std::ifstream file(config_path);
  std::string line;
  int count = 0;
  while (std::getline(file, line)) {
    count++;
  }
  EXPECT_EQ(count, 2);
}

TEST(ComposeCompilerTest, ParseStabilityConfig) {
  TempDir temp;
  auto config_path = temp.path() / "stability.txt";

  // Create stability config
  std::ofstream file(config_path);
  file << "com.example.MyStableClass\n";
  file << "# This is a comment\n";
  file << "\n"; // Empty line
  file << "com.example.AnotherStableClass\n";
  file.close();

  auto result = compose_compiler::parse_stability_config(config_path);

  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 2);
  EXPECT_EQ((*result)[0], "com.example.MyStableClass");
  EXPECT_EQ((*result)[1], "com.example.AnotherStableClass");
}

TEST(ComposeCompilerTest, ParseStabilityConfigFailsForNonexistentFile) {
  auto result = compose_compiler::parse_stability_config("/nonexistent/path.txt");
  EXPECT_FALSE(result.has_value());
}

// Test integration with Kotlin compiler
TEST(ComposeKotlinIntegrationTest, KotlinCompileConfigSupportsComposeConfig) {
  KotlinCompileConfig config;

  ComposeCompilerConfig compose_config;
  compose_config.enabled = true;
  compose_config.version = "1.5.4";
  compose_config.kotlin_version = "1.9.20";

  config.compose_config = compose_config;

  EXPECT_TRUE(config.compose_config.has_value());
  EXPECT_TRUE(config.compose_config->enabled);
  EXPECT_EQ(config.compose_config->version, "1.5.4");
}

TEST(ComposeKotlinIntegrationTest, ComposeConfigIncludedInCompilationHash) {
  KotlinCompileConfig config1;
  config1.language_version = "1.9";
  config1.jvm_target = "17";
  config1.api_version = "1.9";

  KotlinCompileConfig config2 = config1;

  // Without Compose
  std::string hash1 = AndroidKotlinCompiler::compute_compilation_hash(config1);

  // With Compose
  ComposeCompilerConfig compose_config;
  compose_config.enabled = true;
  compose_config.version = "1.5.4";
  compose_config.kotlin_version = "1.9.20";
  config2.compose_config = compose_config;

  std::string hash2 = AndroidKotlinCompiler::compute_compilation_hash(config2);

  EXPECT_NE(hash1, hash2);
}

// Test metrics parsing
TEST(ComposeCompilerTest, ParseMetricsReturnsErrorForNonexistentDirectory) {
  auto result = compose_compiler::parse_compose_metrics("/nonexistent/metrics");
  EXPECT_FALSE(result.has_value());
}

TEST(ComposeCompilerTest, ParseMetricsWithEmptyDirectory) {
  TempDir temp;
  auto metrics_dir = temp.path() / "metrics";
  std::filesystem::create_directories(metrics_dir);

  auto result = compose_compiler::parse_compose_metrics(metrics_dir);

  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result->total_composables, 0);
}

// Test generated classes scanning
TEST(ComposeCompilerTest, ScanGeneratedClassesReturnsEmptyForNonexistentDir) {
  auto classes = compose_compiler::scan_compose_generated_classes("/nonexistent/output");
  EXPECT_TRUE(classes.empty());
}

TEST(ComposeCompilerTest, ScanGeneratedClassesFindsComposeClasses) {
  TempDir temp;
  auto output_dir = temp.path() / "output";
  std::filesystem::create_directories(output_dir);

  // Create mock Compose generated files
  std::ofstream(output_dir / "ComposableSingletons$MainActivity.class").close();
  std::ofstream(output_dir / "MainActivityKt$ComposerImpl.class").close();
  std::ofstream(output_dir / "RegularClass.class").close(); // Should not be included

  auto classes = compose_compiler::scan_compose_generated_classes(output_dir);

  EXPECT_EQ(classes.size(), 2);
}

TEST(ComposeCompilerTest, ScanGeneratedClassesIsDeterministic) {
  TempDir temp;
  auto output_dir = temp.path() / "output";
  std::filesystem::create_directories(output_dir);

  std::ofstream(output_dir / "B_ComposableSingletons.class").close();
  std::ofstream(output_dir / "A_ComposableSingletons.class").close();

  auto classes1 = compose_compiler::scan_compose_generated_classes(output_dir);
  auto classes2 = compose_compiler::scan_compose_generated_classes(output_dir);

  EXPECT_EQ(classes1, classes2);
  // Should be sorted
  EXPECT_TRUE(std::is_sorted(classes1.begin(), classes1.end()));
}

} // namespace horcrux::core::test
