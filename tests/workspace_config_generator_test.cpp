// Horcrux - Workspace Config Generator Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/core/workspace_config_generator.h"

namespace horcrux::core::test {

class WorkspaceConfigGeneratorTestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    test_dir_ = std::filesystem::temp_directory_path() / "horcrux_config_test";
    std::filesystem::create_directories(test_dir_);
  }

  void TearDown() override {
    if (std::filesystem::exists(test_dir_)) {
      std::filesystem::remove_all(test_dir_);
    }
  }

  std::filesystem::path test_dir_;
};

TEST_F(WorkspaceConfigGeneratorTestFixture, ErrorToString) {
  EXPECT_EQ(to_string(ConfigGeneratorError::FileWriteError), "Failed to write file");
  EXPECT_EQ(to_string(ConfigGeneratorError::InvalidConfig), "Invalid configuration");
  EXPECT_EQ(to_string(ConfigGeneratorError::DirectoryError), "Directory error");
}

TEST_F(WorkspaceConfigGeneratorTestFixture, GenerateBasicConfig) {
  GradleProject project;
  project.name = "TestProject";
  project.root_dir = test_dir_;

  GradleBuildConfig build_config;
  build_config.project_type = "android-app";
  build_config.compile_sdk = "34";
  build_config.min_sdk = "21";
  build_config.target_sdk = "34";
  build_config.application_id = "com.example.test";
  build_config.version_name = "1.0";
  build_config.version_code = "1";

  auto output_path = test_dir_ / "horcrux.yaml";
  auto result = WorkspaceConfigGenerator::generate_from_gradle(project, build_config, output_path);

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(std::filesystem::exists(output_path));

  // Read and verify content
  std::ifstream file(output_path);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("TestProject"), std::string::npos);
  EXPECT_NE(content.find("android-app"), std::string::npos);
  EXPECT_NE(content.find("compileSdk: 34"), std::string::npos);
  EXPECT_NE(content.find("minSdk: 21"), std::string::npos);
  EXPECT_NE(content.find("com.example.test"), std::string::npos);
}

TEST_F(WorkspaceConfigGeneratorTestFixture, GenerateConfigWithDependencies) {
  GradleProject project;
  project.name = "DepsProject";
  project.root_dir = test_dir_;

  GradleBuildConfig build_config;
  build_config.project_type = "android-library";

  GradleDependency dep1;
  dep1.group = "androidx.core";
  dep1.name = "core-ktx";
  dep1.version = "1.12.0";
  dep1.configuration = "implementation";
  build_config.dependencies.push_back(dep1);

  GradleDependency dep2;
  dep2.group = "junit";
  dep2.name = "junit";
  dep2.version = "4.13.2";
  dep2.configuration = "testImplementation";
  build_config.dependencies.push_back(dep2);

  auto output_path = test_dir_ / "horcrux.yaml";
  auto result = WorkspaceConfigGenerator::generate_from_gradle(project, build_config, output_path);

  ASSERT_TRUE(result.has_value());

  // Read and verify content
  std::ifstream file(output_path);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("dependencies:"), std::string::npos);
  EXPECT_NE(content.find("core-ktx"), std::string::npos);
  EXPECT_NE(content.find("1.12.0"), std::string::npos);
  EXPECT_NE(content.find("junit"), std::string::npos);
}

TEST_F(WorkspaceConfigGeneratorTestFixture, GenerateConfigWithSourceSets) {
  GradleProject project;
  project.name = "SourceSetProject";
  project.root_dir = test_dir_;

  GradleBuildConfig build_config;
  build_config.project_type = "application";

  GradleSourceSet main_set;
  main_set.name = "main";
  main_set.java_dirs.push_back("src/main/java");
  main_set.resources_dirs.push_back("src/main/resources");
  build_config.source_sets.push_back(main_set);

  GradleSourceSet test_set;
  test_set.name = "test";
  test_set.java_dirs.push_back("src/test/java");
  build_config.source_sets.push_back(test_set);

  auto output_path = test_dir_ / "horcrux.yaml";
  auto result = WorkspaceConfigGenerator::generate_from_gradle(project, build_config, output_path);

  ASSERT_TRUE(result.has_value());

  // Read and verify content
  std::ifstream file(output_path);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("sourceSets:"), std::string::npos);
  EXPECT_NE(content.find("main:"), std::string::npos);
  EXPECT_NE(content.find("src/main/java"), std::string::npos);
  EXPECT_NE(content.find("test:"), std::string::npos);
}

TEST_F(WorkspaceConfigGeneratorTestFixture, GenerateConfigWithBuildVariants) {
  GradleProject project;
  project.name = "VariantProject";
  project.root_dir = test_dir_;

  GradleBuildConfig build_config;
  build_config.project_type = "android-app";

  GradleBuildVariant debug;
  debug.name = "debug";
  debug.build_type = "debug";
  build_config.build_variants.push_back(debug);

  GradleBuildVariant release;
  release.name = "release";
  release.build_type = "release";
  build_config.build_variants.push_back(release);

  auto output_path = test_dir_ / "horcrux.yaml";
  auto result = WorkspaceConfigGenerator::generate_from_gradle(project, build_config, output_path);

  ASSERT_TRUE(result.has_value());

  // Read and verify content
  std::ifstream file(output_path);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("buildVariants:"), std::string::npos);
  EXPECT_NE(content.find("debug:"), std::string::npos);
  EXPECT_NE(content.find("release:"), std::string::npos);
}

TEST_F(WorkspaceConfigGeneratorTestFixture, InvalidOutputPath) {
  GradleProject project;
  project.name = "InvalidPathProject";

  GradleBuildConfig build_config;
  build_config.project_type = "library";

  // Try to write to a non-existent directory without creating it
  auto bad_output = std::filesystem::path("/nonexistent/path/horcrux.yaml");
  auto result = WorkspaceConfigGenerator::generate_from_gradle(project, build_config, bad_output);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ConfigGeneratorError::FileWriteError);
}

TEST_F(WorkspaceConfigGeneratorTestFixture, GenerateJavaApplicationConfig) {
  GradleProject project;
  project.name = "JavaApp";
  project.root_dir = test_dir_;

  GradleBuildConfig build_config;
  build_config.project_type = "application";

  GradleSourceSet main_set;
  main_set.name = "main";
  main_set.java_dirs.push_back("src/main/java");
  build_config.source_sets.push_back(main_set);

  auto output_path = test_dir_ / "horcrux.yaml";
  auto result = WorkspaceConfigGenerator::generate_from_gradle(project, build_config, output_path);

  ASSERT_TRUE(result.has_value());

  // Read and verify content
  std::ifstream file(output_path);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("type: application"), std::string::npos);
  EXPECT_NE(content.find("java_application"), std::string::npos);
}

} // namespace horcrux::core::test
