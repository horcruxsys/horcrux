// Horcrux - Android Java Compiler Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/core/android_java_compiler.h"
#include "../src/core/android_toolchain.h"

namespace horcrux::core::test {

// Helper to create a temporary directory for testing
class TempDir {
public:
  TempDir() {
    path_ = std::filesystem::temp_directory_path() / "horcrux_java_test";
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
TEST(JavaCompilerErrorTest, ErrorToString) {
  EXPECT_FALSE(to_string(JavaCompilerError::CompilerNotFound).empty());
  EXPECT_FALSE(to_string(JavaCompilerError::InvalidSourceFile).empty());
  EXPECT_FALSE(to_string(JavaCompilerError::InvalidClasspath).empty());
  EXPECT_FALSE(to_string(JavaCompilerError::CompilationFailed).empty());
}

// Test classpath helper functions
TEST(JavaClasspathTest, BuildClasspathString) {
  std::vector<std::filesystem::path> classpath = {"/path/to/lib1.jar", "/path/to/lib2.jar"};
  std::string result = java_classpath::build_classpath_string(classpath);

  EXPECT_FALSE(result.empty());
  EXPECT_NE(result.find("lib1.jar"), std::string::npos);
  EXPECT_NE(result.find("lib2.jar"), std::string::npos);

#ifdef _WIN32
  EXPECT_NE(result.find(';'), std::string::npos);
#else
  EXPECT_NE(result.find(':'), std::string::npos);
#endif
}

TEST(JavaClasspathTest, BuildClasspathStringEmpty) {
  std::vector<std::filesystem::path> classpath;
  std::string result = java_classpath::build_classpath_string(classpath);
  EXPECT_TRUE(result.empty());
}

TEST(JavaClasspathTest, ParseClasspathString) {
  char sep = java_classpath::get_path_separator();
  std::string classpath_str = std::string("/path/to/lib1.jar") + sep + "/path/to/lib2.jar";
  auto result = java_classpath::parse_classpath_string(classpath_str);

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].string(), "/path/to/lib1.jar");
  EXPECT_EQ(result[1].string(), "/path/to/lib2.jar");
}

TEST(JavaClasspathTest, GetPathSeparator) {
  char sep = java_classpath::get_path_separator();
#ifdef _WIN32
  EXPECT_EQ(sep, ';');
#else
  EXPECT_EQ(sep, ':');
#endif
}

TEST(JavaClasspathTest, ValidateClasspathReturnsFalseForNonexistent) {
  std::vector<std::filesystem::path> classpath = {"/nonexistent/path/lib.jar"};
  EXPECT_FALSE(java_classpath::validate_classpath(classpath));
}

TEST(JavaClasspathTest, ValidateClasspathReturnsTrueForExisting) {
  TempDir temp;
  auto jar_path = temp.path() / "test.jar";
  std::ofstream jar_file(jar_path);
  jar_file << "fake jar";
  jar_file.close();

  std::vector<std::filesystem::path> classpath = {jar_path};
  EXPECT_TRUE(java_classpath::validate_classpath(classpath));
}

// Test incremental compilation helpers
TEST(JavaIncrementalTest, ComputeSourceHash) {
  TempDir temp;
  auto source_path = temp.path() / "Test.java";
  std::ofstream source(source_path);
  source << "public class Test { }";
  source.close();

  std::string hash = java_incremental::compute_source_hash(source_path);
  EXPECT_FALSE(hash.empty());
  EXPECT_EQ(hash.length(), 64); // SHA-256 is 64 hex characters
}

TEST(JavaIncrementalTest, HasSourceChangedReturnsTrueForDifferentContent) {
  TempDir temp;
  auto source_path = temp.path() / "Test.java";
  std::ofstream source(source_path);
  source << "public class Test { }";
  source.close();

  std::string hash1 = java_incremental::compute_source_hash(source_path);

  // Modify file
  std::ofstream source2(source_path);
  source2 << "public class Test { int x; }";
  source2.close();

  EXPECT_TRUE(java_incremental::has_source_changed(source_path, hash1));
}

TEST(JavaIncrementalTest, HasSourceChangedReturnsFalseForSameContent) {
  TempDir temp;
  auto source_path = temp.path() / "Test.java";
  std::ofstream source(source_path);
  source << "public class Test { }";
  source.close();

  std::string hash = java_incremental::compute_source_hash(source_path);
  EXPECT_FALSE(java_incremental::has_source_changed(source_path, hash));
}

TEST(JavaIncrementalTest, SaveAndLoadCompilationState) {
  TempDir temp;
  auto state_file = temp.path() / ".horcrux_state";

  JavaCompileConfig config;
  config.output_dir = temp.path();
  std::string hash = "abc123def456";

  EXPECT_TRUE(java_incremental::save_compilation_state(state_file, config, hash));

  auto loaded = java_incremental::load_compilation_state(state_file);
  ASSERT_TRUE(loaded.has_value());
  EXPECT_EQ(*loaded, hash);
}

TEST(JavaIncrementalTest, LoadCompilationStateReturnsNulloptForNonexistent) {
  auto loaded = java_incremental::load_compilation_state("/nonexistent/state");
  EXPECT_FALSE(loaded.has_value());
}

// Test Java source parsing
TEST(JavaSourceParsingTest, ParseJavaSourceExtractsPackage) {
  TempDir temp;
  auto source_path = temp.path() / "Test.java";
  std::ofstream source(source_path);
  source << "package com.example.test;\n";
  source << "public class Test { }";
  source.close();

  auto result = AndroidJavaCompiler::parse_java_source(source_path);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->package_name, "com.example.test");
}

TEST(JavaSourceParsingTest, ParseJavaSourceExtractsImports) {
  TempDir temp;
  auto source_path = temp.path() / "Test.java";
  std::ofstream source(source_path);
  source << "import java.util.List;\n";
  source << "import java.util.ArrayList;\n";
  source << "public class Test { }";
  source.close();

  auto result = AndroidJavaCompiler::parse_java_source(source_path);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->imports.size(), 2);
  EXPECT_NE(std::find(result->imports.begin(), result->imports.end(), "java.util.List"),
            result->imports.end());
  EXPECT_NE(std::find(result->imports.begin(), result->imports.end(), "java.util.ArrayList"),
            result->imports.end());
}

TEST(JavaSourceParsingTest, ParseJavaSourceComputesHash) {
  TempDir temp;
  auto source_path = temp.path() / "Test.java";
  std::ofstream source(source_path);
  source << "public class Test { }";
  source.close();

  auto result = AndroidJavaCompiler::parse_java_source(source_path);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->content_hash.empty());
  EXPECT_EQ(result->content_hash.length(), 64); // SHA-256
}

TEST(JavaSourceParsingTest, ParseJavaSourceFailsForNonexistent) {
  auto result = AndroidJavaCompiler::parse_java_source("/nonexistent/Test.java");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), JavaCompilerError::InvalidSourceFile);
}

// Test config validation
TEST(JavaCompilerConfigTest, ValidateConfigFailsForEmptySources) {
  JavaCompileConfig config;
  config.output_dir = "/tmp/output";

  auto result = AndroidJavaCompiler::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), JavaCompilerError::InvalidConfiguration);
}

TEST(JavaCompilerConfigTest, ValidateConfigFailsForNonexistentSource) {
  JavaCompileConfig config;
  config.output_dir = "/tmp/output";

  JavaSourceFile source;
  source.path = "/nonexistent/Test.java";
  config.sources.push_back(source);

  auto result = AndroidJavaCompiler::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), JavaCompilerError::InvalidSourceFile);
}

TEST(JavaCompilerConfigTest, ValidateConfigSucceedsForValidConfig) {
  TempDir temp;
  auto source_path = temp.path() / "Test.java";
  std::ofstream source(source_path);
  source << "public class Test { }";
  source.close();

  JavaCompileConfig config;
  config.output_dir = temp.path() / "output";

  JavaSourceFile source_file;
  source_file.path = source_path;
  config.sources.push_back(source_file);

  auto result = AndroidJavaCompiler::validate_config(config);
  EXPECT_TRUE(result.has_value());
}

// Test compilation hash computation
TEST(JavaCompilerTest, ComputeCompilationHashIsDeterministic) {
  TempDir temp;
  auto source_path = temp.path() / "Test.java";
  std::ofstream source(source_path);
  source << "public class Test { }";
  source.close();

  JavaCompileConfig config;
  config.output_dir = temp.path() / "output";
  config.source_version = "11";
  config.target_version = "11";

  JavaSourceFile source_file;
  source_file.path = source_path;
  source_file.content_hash = "abc123";
  config.sources.push_back(source_file);

  std::string hash1 = AndroidJavaCompiler::compute_compilation_hash(config);
  std::string hash2 = AndroidJavaCompiler::compute_compilation_hash(config);

  EXPECT_EQ(hash1, hash2);
  EXPECT_FALSE(hash1.empty());
  EXPECT_EQ(hash1.length(), 64); // SHA-256
}

TEST(JavaCompilerTest, ComputeCompilationHashChangesWhenConfigChanges) {
  TempDir temp;
  auto source_path = temp.path() / "Test.java";
  std::ofstream source(source_path);
  source << "public class Test { }";
  source.close();

  JavaCompileConfig config1;
  config1.output_dir = temp.path() / "output";
  config1.source_version = "11";

  JavaSourceFile source_file;
  source_file.path = source_path;
  source_file.content_hash = "abc123";
  config1.sources.push_back(source_file);

  JavaCompileConfig config2 = config1;
  config2.source_version = "17";

  std::string hash1 = AndroidJavaCompiler::compute_compilation_hash(config1);
  std::string hash2 = AndroidJavaCompiler::compute_compilation_hash(config2);

  EXPECT_NE(hash1, hash2);
}

// Test mock Android toolchain creation
TEST(JavaCompilerTest, CompilerRequiresValidToolchain) {
  TempDir temp;
  TempDir java_temp;

  // Create minimal SDK
  std::filesystem::create_directories(temp.path() / "platforms");

  // Create fake Java SDK
  std::filesystem::create_directories(java_temp.path() / "bin");
#ifdef _WIN32
  std::filesystem::path javac_path = java_temp.path() / "bin" / "javac.exe";
#else
  std::filesystem::path javac_path = java_temp.path() / "bin" / "javac";
#endif
  std::ofstream javac_file(javac_path);
  javac_file << "#!/bin/bash\n";
  javac_file.close();

  auto toolchain_result = AndroidToolchainDetector::detect(temp.path(), std::nullopt, java_temp.path());
  ASSERT_TRUE(toolchain_result.has_value());

  AndroidJavaCompiler compiler(*toolchain_result);

  // This test just verifies compiler creation doesn't crash
  EXPECT_TRUE(true);
}

} // namespace horcrux::core::test
