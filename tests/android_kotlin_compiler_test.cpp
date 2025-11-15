// Horcrux - Android Kotlin Compiler Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/core/android_kotlin_compiler.h"
#include "../src/core/android_toolchain.h"

namespace horcrux::core::test {

// Helper to create a temporary directory for testing
class TempDir {
public:
  TempDir() {
    path_ = std::filesystem::temp_directory_path() / "horcrux_kotlin_test";
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

// Test error to string conversion
TEST(KotlinCompilerErrorTest, ErrorToString) {
  EXPECT_FALSE(to_string(KotlinCompilerError::CompilerNotFound).empty());
  EXPECT_FALSE(to_string(KotlinCompilerError::InvalidSourceFile).empty());
  EXPECT_FALSE(to_string(KotlinCompilerError::InvalidClasspath).empty());
  EXPECT_FALSE(to_string(KotlinCompilerError::CompilationFailed).empty());
}

// Test Kotlin/Java interop helpers
TEST(KotlinJavaInteropTest, IsKotlinFile) {
  EXPECT_TRUE(kotlin_java_interop::is_kotlin_file("/path/to/Test.kt"));
  EXPECT_FALSE(kotlin_java_interop::is_kotlin_file("/path/to/Test.java"));
  EXPECT_FALSE(kotlin_java_interop::is_kotlin_file("/path/to/Test.cpp"));
}

TEST(KotlinJavaInteropTest, IsJavaFile) {
  EXPECT_TRUE(kotlin_java_interop::is_java_file("/path/to/Test.java"));
  EXPECT_FALSE(kotlin_java_interop::is_java_file("/path/to/Test.kt"));
  EXPECT_FALSE(kotlin_java_interop::is_java_file("/path/to/Test.cpp"));
}

TEST(KotlinJavaInteropTest, SeparateSources) {
  std::vector<std::filesystem::path> sources = {"/path/to/Test.kt", "/path/to/Main.java",
                                                "/path/to/Utils.kt", "/path/to/Helper.java"};

  auto [kotlin, java] = kotlin_java_interop::separate_sources(sources);

  EXPECT_EQ(kotlin.size(), 2);
  EXPECT_EQ(java.size(), 2);
}

TEST(KotlinJavaInteropTest, ValidateCompilationOrder) {
  std::vector<std::filesystem::path> kotlin_sources = {"/path/to/Test.kt"};
  std::vector<std::filesystem::path> java_sources = {"/path/to/Main.java"};

  EXPECT_TRUE(kotlin_java_interop::validate_compilation_order(kotlin_sources, java_sources));
}

// Test incremental compilation helpers
TEST(KotlinIncrementalTest, ComputeSourceHash) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "class Test { }";
  source.close();

  std::string hash = kotlin_incremental::compute_source_hash(source_path);
  EXPECT_FALSE(hash.empty());
  EXPECT_EQ(hash.length(), 64); // SHA-256 is 64 hex characters
}

TEST(KotlinIncrementalTest, HasSourceChangedReturnsTrueForDifferentContent) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "class Test { }";
  source.close();

  std::string hash1 = kotlin_incremental::compute_source_hash(source_path);

  // Modify file
  std::ofstream source2(source_path);
  source2 << "class Test { val x: Int = 0 }";
  source2.close();

  EXPECT_TRUE(kotlin_incremental::has_source_changed(source_path, hash1));
}

TEST(KotlinIncrementalTest, HasSourceChangedReturnsFalseForSameContent) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "class Test { }";
  source.close();

  std::string hash = kotlin_incremental::compute_source_hash(source_path);
  EXPECT_FALSE(kotlin_incremental::has_source_changed(source_path, hash));
}

TEST(KotlinIncrementalTest, SaveAndLoadCompilationState) {
  TempDir temp;
  auto state_file = temp.path() / ".horcrux_kotlin_state";

  KotlinCompileConfig config;
  config.output_dir = temp.path();
  std::string hash = "xyz789abc123";

  EXPECT_TRUE(kotlin_incremental::save_compilation_state(state_file, config, hash));

  auto loaded = kotlin_incremental::load_compilation_state(state_file);
  ASSERT_TRUE(loaded.has_value());
  EXPECT_EQ(*loaded, hash);
}

TEST(KotlinIncrementalTest, LoadCompilationStateReturnsNulloptForNonexistent) {
  auto loaded = kotlin_incremental::load_compilation_state("/nonexistent/state");
  EXPECT_FALSE(loaded.has_value());
}

// Test Kotlin source parsing
TEST(KotlinSourceParsingTest, ParseKotlinSourceExtractsPackage) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "package com.example.test\n";
  source << "class Test { }";
  source.close();

  auto result = AndroidKotlinCompiler::parse_kotlin_source(source_path);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->package_name, "com.example.test");
}

TEST(KotlinSourceParsingTest, ParseKotlinSourceExtractsImports) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "import java.util.List\n";
  source << "import kotlin.collections.ArrayList\n";
  source << "class Test { }";
  source.close();

  auto result = AndroidKotlinCompiler::parse_kotlin_source(source_path);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->imports.size(), 2);
  EXPECT_NE(std::find(result->imports.begin(), result->imports.end(), "java.util.List"),
            result->imports.end());
  EXPECT_NE(
      std::find(result->imports.begin(), result->imports.end(), "kotlin.collections.ArrayList"),
      result->imports.end());
}

TEST(KotlinSourceParsingTest, ParseKotlinSourceComputesHash) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "class Test { }";
  source.close();

  auto result = AndroidKotlinCompiler::parse_kotlin_source(source_path);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->content_hash.empty());
  EXPECT_EQ(result->content_hash.length(), 64); // SHA-256
}

TEST(KotlinSourceParsingTest, ParseKotlinSourceFailsForNonexistent) {
  auto result = AndroidKotlinCompiler::parse_kotlin_source("/nonexistent/Test.kt");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), KotlinCompilerError::InvalidSourceFile);
}

// Test config validation
TEST(KotlinCompilerConfigTest, ValidateConfigFailsForEmptySources) {
  KotlinCompileConfig config;
  config.output_dir = "/tmp/output";

  auto result = AndroidKotlinCompiler::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), KotlinCompilerError::InvalidConfiguration);
}

TEST(KotlinCompilerConfigTest, ValidateConfigFailsForNonexistentKotlinSource) {
  KotlinCompileConfig config;
  config.output_dir = "/tmp/output";

  KotlinSourceFile source;
  source.path = "/nonexistent/Test.kt";
  config.kotlin_sources.push_back(source);

  auto result = AndroidKotlinCompiler::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), KotlinCompilerError::InvalidSourceFile);
}

TEST(KotlinCompilerConfigTest, ValidateConfigFailsForNonexistentJavaSource) {
  KotlinCompileConfig config;
  config.output_dir = "/tmp/output";
  config.java_sources.push_back("/nonexistent/Test.java");

  auto result = AndroidKotlinCompiler::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), KotlinCompilerError::InvalidSourceFile);
}

TEST(KotlinCompilerConfigTest, ValidateConfigSucceedsForValidKotlinConfig) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "class Test { }";
  source.close();

  KotlinCompileConfig config;
  config.output_dir = temp.path() / "output";

  KotlinSourceFile source_file;
  source_file.path = source_path;
  config.kotlin_sources.push_back(source_file);

  auto result = AndroidKotlinCompiler::validate_config(config);
  EXPECT_TRUE(result.has_value());
}

TEST(KotlinCompilerConfigTest, ValidateConfigSucceedsForMixedKotlinJava) {
  TempDir temp;
  auto kt_path = temp.path() / "Test.kt";
  std::ofstream kt_source(kt_path);
  kt_source << "class Test { }";
  kt_source.close();

  auto java_path = temp.path() / "Main.java";
  std::ofstream java_source(java_path);
  java_source << "public class Main { }";
  java_source.close();

  KotlinCompileConfig config;
  config.output_dir = temp.path() / "output";

  KotlinSourceFile kt_file;
  kt_file.path = kt_path;
  config.kotlin_sources.push_back(kt_file);
  config.java_sources.push_back(java_path);

  auto result = AndroidKotlinCompiler::validate_config(config);
  EXPECT_TRUE(result.has_value());
}

// Test compilation hash computation
TEST(KotlinCompilerTest, ComputeCompilationHashIsDeterministic) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "class Test { }";
  source.close();

  KotlinCompileConfig config;
  config.output_dir = temp.path() / "output";
  config.language_version = "1.9";
  config.jvm_target = "11";

  KotlinSourceFile source_file;
  source_file.path = source_path;
  source_file.content_hash = "xyz123";
  config.kotlin_sources.push_back(source_file);

  std::string hash1 = AndroidKotlinCompiler::compute_compilation_hash(config);
  std::string hash2 = AndroidKotlinCompiler::compute_compilation_hash(config);

  EXPECT_EQ(hash1, hash2);
  EXPECT_FALSE(hash1.empty());
  EXPECT_EQ(hash1.length(), 64); // SHA-256
}

TEST(KotlinCompilerTest, ComputeCompilationHashChangesWhenConfigChanges) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "class Test { }";
  source.close();

  KotlinCompileConfig config1;
  config1.output_dir = temp.path() / "output";
  config1.language_version = "1.8";

  KotlinSourceFile source_file;
  source_file.path = source_path;
  source_file.content_hash = "xyz123";
  config1.kotlin_sources.push_back(source_file);

  KotlinCompileConfig config2 = config1;
  config2.language_version = "1.9";

  std::string hash1 = AndroidKotlinCompiler::compute_compilation_hash(config1);
  std::string hash2 = AndroidKotlinCompiler::compute_compilation_hash(config2);

  EXPECT_NE(hash1, hash2);
}

TEST(KotlinCompilerTest, ComputeCompilationHashIncludesKaptConfig) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "class Test { }";
  source.close();

  KotlinCompileConfig config1;
  config1.output_dir = temp.path() / "output";

  KotlinSourceFile source_file;
  source_file.path = source_path;
  source_file.content_hash = "xyz123";
  config1.kotlin_sources.push_back(source_file);

  KotlinCompileConfig config2 = config1;
  config2.kapt_config = KotlinCompileConfig::KaptConfig();
  config2.kapt_config->enabled = true;

  std::string hash1 = AndroidKotlinCompiler::compute_compilation_hash(config1);
  std::string hash2 = AndroidKotlinCompiler::compute_compilation_hash(config2);

  EXPECT_NE(hash1, hash2);
}

TEST(KotlinCompilerTest, ComputeCompilationHashIncludesKspConfig) {
  TempDir temp;
  auto source_path = temp.path() / "Test.kt";
  std::ofstream source(source_path);
  source << "class Test { }";
  source.close();

  KotlinCompileConfig config1;
  config1.output_dir = temp.path() / "output";

  KotlinSourceFile source_file;
  source_file.path = source_path;
  source_file.content_hash = "xyz123";
  config1.kotlin_sources.push_back(source_file);

  KotlinCompileConfig config2 = config1;
  config2.ksp_config = KotlinCompileConfig::KspConfig();
  config2.ksp_config->enabled = true;

  std::string hash1 = AndroidKotlinCompiler::compute_compilation_hash(config1);
  std::string hash2 = AndroidKotlinCompiler::compute_compilation_hash(config2);

  EXPECT_NE(hash1, hash2);
}

// Test mock Android toolchain creation
TEST(KotlinCompilerTest, CompilerRequiresValidToolchain) {
  TempDir temp;

  // Create minimal SDK
  std::filesystem::create_directories(temp.path() / "platforms");

  auto toolchain_result = AndroidToolchainDetector::detect(temp.path());
  ASSERT_TRUE(toolchain_result.has_value());

  AndroidKotlinCompiler compiler(*toolchain_result);

  // This test just verifies compiler creation doesn't crash
  EXPECT_TRUE(true);
}

TEST(KotlinCompilerTest, SetKotlincPath) {
  TempDir temp;

  // Create minimal SDK
  std::filesystem::create_directories(temp.path() / "platforms");

  auto toolchain_result = AndroidToolchainDetector::detect(temp.path());
  ASSERT_TRUE(toolchain_result.has_value());

  AndroidKotlinCompiler compiler(*toolchain_result);
  compiler.set_kotlinc_path("/custom/path/to/kotlinc");

  // This test just verifies setting path doesn't crash
  EXPECT_TRUE(true);
}

} // namespace horcrux::core::test
