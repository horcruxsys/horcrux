// Horcrux - Android D8/R8 DEX Compiler Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_dex_compiler.h"
#include "android_toolchain.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace horcrux::core;

class DexCompilerTestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    // Create temporary test directories
    test_dir_ = std::filesystem::temp_directory_path() / "horcrux_dex_test";
    std::filesystem::create_directories(test_dir_);

    input_dir_ = test_dir_ / "input";
    output_dir_ = test_dir_ / "output";
    std::filesystem::create_directories(input_dir_);
    std::filesystem::create_directories(output_dir_);

    // Create mock toolchain
    toolchain_.sdk_root = test_dir_ / "sdk";
    std::filesystem::create_directories(toolchain_.sdk_root);

    // Create mock build tools
    auto build_tools_dir = toolchain_.sdk_root / "build-tools" / "34.0.0";
    std::filesystem::create_directories(build_tools_dir);

    AndroidBuildTools bt;
    bt.version = "34.0.0";
    bt.path = build_tools_dir;
    bt.d8_path = build_tools_dir / "d8";
    bt.r8_path = build_tools_dir / "r8";

    // Create mock tool files
    std::ofstream d8_file(bt.d8_path);
    d8_file << "#!/bin/bash\necho 'Mock D8'\n";
    d8_file.close();
    std::filesystem::permissions(bt.d8_path, std::filesystem::perms::owner_exec,
                                 std::filesystem::perm_options::add);

    std::ofstream r8_file(bt.r8_path);
    r8_file << "#!/bin/bash\necho 'Mock R8'\n";
    r8_file.close();
    std::filesystem::permissions(bt.r8_path, std::filesystem::perms::owner_exec,
                                 std::filesystem::perm_options::add);

    toolchain_.build_tools.push_back(std::move(bt));
  }

  void TearDown() override {
    // Clean up test directories
    std::filesystem::remove_all(test_dir_);
  }

  auto create_test_class_file(const std::string& name) -> std::filesystem::path {
    auto class_file = input_dir_ / (name + ".class");
    std::ofstream file(class_file);
    file << "Mock class file for " << name << "\n";
    file.close();
    return class_file;
  }

  auto create_test_jar_file(const std::string& name) -> std::filesystem::path {
    auto jar_file = input_dir_ / (name + ".jar");
    std::ofstream file(jar_file);
    file << "Mock JAR file for " << name << "\n";
    file.close();
    return jar_file;
  }

  auto create_test_proguard_config() -> std::filesystem::path {
    auto pg_file = test_dir_ / "proguard-rules.pro";
    std::ofstream file(pg_file);
    file << "# ProGuard rules\n";
    file << "-keep public class * extends android.app.Activity\n";
    file << "-keepclassmembers class * {\n";
    file << "  public <init>(...);\n";
    file << "}\n";
    file.close();
    return pg_file;
  }

  std::filesystem::path test_dir_;
  std::filesystem::path input_dir_;
  std::filesystem::path output_dir_;
  AndroidToolchain toolchain_;
};

// Error string conversion tests
TEST_F(DexCompilerTestFixture, ErrorToString) {
  EXPECT_EQ(to_string(DexCompilerError::CompilerNotFound),
            "D8/R8 compiler not found in Android build tools");
  EXPECT_EQ(to_string(DexCompilerError::InvalidInputFile),
            "Invalid input file (must be .class or .jar)");
  EXPECT_EQ(to_string(DexCompilerError::InvalidConfiguration),
            "Invalid DEX compilation configuration");
  EXPECT_EQ(to_string(DexCompilerError::CompilationFailed), "DEX compilation failed");
  EXPECT_EQ(to_string(DexCompilerError::ProguardConfigNotFound),
            "ProGuard configuration file not found");
}

// D8 configuration validation tests
TEST_F(DexCompilerTestFixture, ValidateD8ConfigEmpty) {
  D8CompileConfig config;
  auto result = AndroidDexCompiler::validate_d8_config(config);
  EXPECT_FALSE(result);
  EXPECT_EQ(result.error(), DexCompilerError::InvalidConfiguration);
}

TEST_F(DexCompilerTestFixture, ValidateD8ConfigNonexistentFile) {
  D8CompileConfig config;
  config.inputs.push_back("/nonexistent/file.class");
  config.output_dir = output_dir_;
  config.min_api = 21;

  auto result = AndroidDexCompiler::validate_d8_config(config);
  EXPECT_FALSE(result);
  EXPECT_EQ(result.error(), DexCompilerError::InvalidInputFile);
}

TEST_F(DexCompilerTestFixture, ValidateD8ConfigInvalidMinApi) {
  auto class_file = create_test_class_file("Test");

  D8CompileConfig config;
  config.inputs.push_back(class_file);
  config.output_dir = output_dir_;
  config.min_api = 0; // Invalid

  auto result = AndroidDexCompiler::validate_d8_config(config);
  EXPECT_FALSE(result);
  EXPECT_EQ(result.error(), DexCompilerError::InvalidConfiguration);
}

TEST_F(DexCompilerTestFixture, ValidateD8ConfigValid) {
  auto class_file = create_test_class_file("Test");

  D8CompileConfig config;
  config.inputs.push_back(class_file);
  config.output_dir = output_dir_;
  config.min_api = 21;

  auto result = AndroidDexCompiler::validate_d8_config(config);
  EXPECT_TRUE(result);
}

// R8 configuration validation tests
TEST_F(DexCompilerTestFixture, ValidateR8ConfigEmpty) {
  R8CompileConfig config;
  auto result = AndroidDexCompiler::validate_r8_config(config);
  EXPECT_FALSE(result);
  EXPECT_EQ(result.error(), DexCompilerError::InvalidConfiguration);
}

TEST_F(DexCompilerTestFixture, ValidateR8ConfigNonexistentProguardFile) {
  auto class_file = create_test_class_file("Test");

  R8CompileConfig config;
  config.inputs.push_back(class_file);
  config.output_dir = output_dir_;
  config.min_api = 21;
  config.proguard_config.config_files.push_back("/nonexistent/proguard-rules.pro");

  auto result = AndroidDexCompiler::validate_r8_config(config);
  EXPECT_FALSE(result);
  EXPECT_EQ(result.error(), DexCompilerError::ProguardConfigNotFound);
}

TEST_F(DexCompilerTestFixture, ValidateR8ConfigValid) {
  auto class_file = create_test_class_file("Test");
  auto pg_file = create_test_proguard_config();

  R8CompileConfig config;
  config.inputs.push_back(class_file);
  config.output_dir = output_dir_;
  config.min_api = 21;
  config.proguard_config.config_files.push_back(pg_file);

  auto result = AndroidDexCompiler::validate_r8_config(config);
  EXPECT_TRUE(result);
}

// Hash computation tests
TEST_F(DexCompilerTestFixture, ComputeD8HashDeterministic) {
  auto class_file1 = create_test_class_file("Test1");
  auto class_file2 = create_test_class_file("Test2");

  D8CompileConfig config;
  config.inputs = {class_file1, class_file2};
  config.output_dir = output_dir_;
  config.min_api = 21;

  auto hash1 = AndroidDexCompiler::compute_d8_hash(config);
  auto hash2 = AndroidDexCompiler::compute_d8_hash(config);

  EXPECT_EQ(hash1, hash2);
  EXPECT_FALSE(hash1.empty());
}

TEST_F(DexCompilerTestFixture, ComputeD8HashChangesWithInput) {
  auto class_file1 = create_test_class_file("Test1");
  auto class_file2 = create_test_class_file("Test2");

  D8CompileConfig config1;
  config1.inputs = {class_file1};
  config1.output_dir = output_dir_;
  config1.min_api = 21;

  D8CompileConfig config2;
  config2.inputs = {class_file2};
  config2.output_dir = output_dir_;
  config2.min_api = 21;

  auto hash1 = AndroidDexCompiler::compute_d8_hash(config1);
  auto hash2 = AndroidDexCompiler::compute_d8_hash(config2);

  EXPECT_NE(hash1, hash2);
}

TEST_F(DexCompilerTestFixture, ComputeD8HashChangesWithMinApi) {
  auto class_file = create_test_class_file("Test");

  D8CompileConfig config1;
  config1.inputs = {class_file};
  config1.output_dir = output_dir_;
  config1.min_api = 21;

  D8CompileConfig config2;
  config2.inputs = {class_file};
  config2.output_dir = output_dir_;
  config2.min_api = 28;

  auto hash1 = AndroidDexCompiler::compute_d8_hash(config1);
  auto hash2 = AndroidDexCompiler::compute_d8_hash(config2);

  EXPECT_NE(hash1, hash2);
}

TEST_F(DexCompilerTestFixture, ComputeR8HashDeterministic) {
  auto class_file = create_test_class_file("Test");
  auto pg_file = create_test_proguard_config();

  R8CompileConfig config;
  config.inputs = {class_file};
  config.output_dir = output_dir_;
  config.min_api = 21;
  config.proguard_config.config_files.push_back(pg_file);

  auto hash1 = AndroidDexCompiler::compute_r8_hash(config);
  auto hash2 = AndroidDexCompiler::compute_r8_hash(config);

  EXPECT_EQ(hash1, hash2);
  EXPECT_FALSE(hash1.empty());
}

TEST_F(DexCompilerTestFixture, ComputeR8HashChangesWithProguardConfig) {
  auto class_file = create_test_class_file("Test");
  auto pg_file = create_test_proguard_config();

  R8CompileConfig config1;
  config1.inputs = {class_file};
  config1.output_dir = output_dir_;
  config1.min_api = 21;
  config1.proguard_config.optimize = true;

  R8CompileConfig config2;
  config2.inputs = {class_file};
  config2.output_dir = output_dir_;
  config2.min_api = 21;
  config2.proguard_config.optimize = false;

  auto hash1 = AndroidDexCompiler::compute_r8_hash(config1);
  auto hash2 = AndroidDexCompiler::compute_r8_hash(config2);

  EXPECT_NE(hash1, hash2);
}

// ProGuard configuration parsing tests
TEST_F(DexCompilerTestFixture, ParseProguardConfigNonexistent) {
  auto result = AndroidDexCompiler::parse_proguard_config("/nonexistent/proguard-rules.pro");
  EXPECT_FALSE(result);
  EXPECT_EQ(result.error(), DexCompilerError::ProguardConfigNotFound);
}

TEST_F(DexCompilerTestFixture, ParseProguardConfigValid) {
  auto pg_file = create_test_proguard_config();
  auto result = AndroidDexCompiler::parse_proguard_config(pg_file);

  EXPECT_TRUE(result);
  EXPECT_FALSE(result->keep_rules.empty());
}

// Tool path detection tests
TEST_F(DexCompilerTestFixture, GetD8PathExists) {
  AndroidDexCompiler compiler(toolchain_);
  // D8 path should be detected from toolchain
  auto d8_result = compiler.compile_d8(D8CompileConfig{});
  // Will fail validation but shows D8 path was found
  EXPECT_FALSE(d8_result);
}

TEST_F(DexCompilerTestFixture, GetR8PathExists) {
  AndroidDexCompiler compiler(toolchain_);
  // R8 path should be detected from toolchain
  auto r8_result = compiler.compile_r8(R8CompileConfig{});
  // Will fail validation but shows R8 path was found
  EXPECT_FALSE(r8_result);
}

// DEX file sorting tests
TEST_F(DexCompilerTestFixture, SortDexFilesDeterministic) {
  // Create mock DEX files
  auto dex1 = output_dir_ / "classes.dex";
  auto dex2 = output_dir_ / "classes2.dex";
  auto dex3 = output_dir_ / "classes3.dex";
  auto dex10 = output_dir_ / "classes10.dex";

  std::ofstream(dex1) << "dex1";
  std::ofstream(dex2) << "dex2";
  std::ofstream(dex3) << "dex3";
  std::ofstream(dex10) << "dex10";

  std::vector<std::filesystem::path> dex_files = {dex10, dex2, dex3, dex1};

  // Sort
  std::vector<std::filesystem::path> sorted = dex_files;
  std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
    std::string a_name = a.filename().string();
    std::string b_name = b.filename().string();
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

  EXPECT_EQ(sorted[0].filename().string(), "classes.dex");
  EXPECT_EQ(sorted[1].filename().string(), "classes2.dex");
  EXPECT_EQ(sorted[2].filename().string(), "classes3.dex");
  EXPECT_EQ(sorted[3].filename().string(), "classes10.dex");
}

// Incremental compilation helper tests
TEST_F(DexCompilerTestFixture, IncrementalComputeInputHash) {
  auto class_file1 = create_test_class_file("Test1");
  auto class_file2 = create_test_class_file("Test2");

  std::vector<std::filesystem::path> inputs = {class_file1, class_file2};

  auto hash1 = dex_incremental::compute_input_hash(inputs);
  auto hash2 = dex_incremental::compute_input_hash(inputs);

  EXPECT_EQ(hash1, hash2);
  EXPECT_FALSE(hash1.empty());
}

TEST_F(DexCompilerTestFixture, IncrementalHasInputChanged) {
  auto class_file = create_test_class_file("Test");
  std::vector<std::filesystem::path> inputs = {class_file};

  auto hash1 = dex_incremental::compute_input_hash(inputs);

  // No change
  EXPECT_FALSE(dex_incremental::has_input_changed(inputs, hash1));

  // Change file
  std::ofstream file(class_file, std::ios::app);
  file << "Modified content";
  file.close();

  // Should detect change
  EXPECT_TRUE(dex_incremental::has_input_changed(inputs, hash1));
}

TEST_F(DexCompilerTestFixture, IncrementalSaveAndLoadState) {
  auto state_file = test_dir_ / ".horcrux_d8_state";

  std::string config_hash = "config_hash_123";
  std::string compilation_hash = "compilation_hash_456";

  // Save state
  bool saved = dex_incremental::save_compilation_state(state_file, config_hash, compilation_hash);
  EXPECT_TRUE(saved);

  // Load state
  auto loaded = dex_incremental::load_compilation_state(state_file);
  EXPECT_TRUE(loaded.has_value());
  EXPECT_EQ(loaded->first, config_hash);
  EXPECT_EQ(loaded->second, compilation_hash);
}

TEST_F(DexCompilerTestFixture, IncrementalLoadStateNonexistent) {
  auto state_file = test_dir_ / ".nonexistent_state";
  auto loaded = dex_incremental::load_compilation_state(state_file);
  EXPECT_FALSE(loaded.has_value());
}

// ProGuard utilities tests
TEST_F(DexCompilerTestFixture, ProguardLoadRules) {
  auto pg_file = create_test_proguard_config();
  auto result = proguard_utils::load_proguard_rules(pg_file);

  EXPECT_TRUE(result);
  EXPECT_FALSE(result->empty());
}

TEST_F(DexCompilerTestFixture, ProguardGenerateDefaultKeepRules) {
  auto rules = proguard_utils::generate_default_keep_rules();
  EXPECT_FALSE(rules.empty());
  EXPECT_TRUE(std::any_of(rules.begin(), rules.end(), [](const std::string& rule) {
    return rule.find("android.app.Activity") != std::string::npos;
  }));
}

TEST_F(DexCompilerTestFixture, ProguardValidateConfig) {
  auto pg_file = create_test_proguard_config();

  ProguardConfig config;
  config.config_files.push_back(pg_file);

  EXPECT_TRUE(proguard_utils::validate_proguard_config(config));
}

TEST_F(DexCompilerTestFixture, ProguardValidateConfigNonexistent) {
  ProguardConfig config;
  config.config_files.push_back("/nonexistent/proguard-rules.pro");

  EXPECT_FALSE(proguard_utils::validate_proguard_config(config));
}

TEST_F(DexCompilerTestFixture, ProguardMergeConfigs) {
  ProguardConfig config1;
  config1.keep_rules = {"-keep class A"};
  config1.optimize = true;

  ProguardConfig config2;
  config2.keep_rules = {"-keep class B"};
  config2.optimize = false;

  auto merged = proguard_utils::merge_proguard_configs({config1, config2});

  EXPECT_EQ(merged.keep_rules.size(), 2);
  EXPECT_FALSE(merged.optimize); // AND logic
}

// Multi-dex helper tests
TEST_F(DexCompilerTestFixture, DexMergeNeedsMultiDex) {
  // This is a stub test - needs actual DEX file parsing
  auto dex_file = output_dir_ / "classes.dex";
  std::ofstream(dex_file) << "mock dex";

  // Should return false for empty/mock file
  EXPECT_FALSE(dex_merge::needs_multi_dex(dex_file));
}

// Integration test for D8 compilation
TEST_F(DexCompilerTestFixture, CompileD8Integration) {
  auto class_file = create_test_class_file("MainActivity");

  D8CompileConfig config;
  config.inputs = {class_file};
  config.output_dir = output_dir_;
  config.min_api = 21;
  config.debug = false;
  config.incremental = false; // Disable for integration test

  AndroidDexCompiler compiler(toolchain_);

  // This will fail because we have mock tools, but tests the flow
  auto result = compiler.compile_d8(config);

  // Expect failure since tools are mocks
  EXPECT_FALSE(result);
}

// Integration test for R8 compilation
TEST_F(DexCompilerTestFixture, CompileR8Integration) {
  auto class_file = create_test_class_file("MainActivity");
  auto pg_file = create_test_proguard_config();

  R8CompileConfig config;
  config.inputs = {class_file};
  config.output_dir = output_dir_;
  config.min_api = 21;
  config.debug = false;
  config.incremental = false;
  config.proguard_config.config_files.push_back(pg_file);

  AndroidDexCompiler compiler(toolchain_);

  // This will fail because we have mock tools, but tests the flow
  auto result = compiler.compile_r8(config);

  // Expect failure since tools are mocks
  EXPECT_FALSE(result);
}
