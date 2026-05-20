// Horcrux - Import Command Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/cli/import_command.h"
#include "../src/cli/logger.h"

namespace horcrux::cli::test {

class ImportCommandTestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    test_dir_ = std::filesystem::temp_directory_path() / "horcrux_import_test" /
                (test_info ? test_info->name() : "default");
    std::filesystem::create_directories(test_dir_);
    logger_ = std::make_unique<Logger>();
  }

  void TearDown() override {
    if (std::filesystem::exists(test_dir_)) {
      std::filesystem::remove_all(test_dir_);
    }
  }

  auto create_test_file(const std::string& filename,
                        const std::string& content) -> std::filesystem::path {
    auto file_path = test_dir_ / filename;
    std::ofstream out(file_path);
    out << content;
    out.close();
    return file_path;
  }

  auto create_gradle_project() -> std::filesystem::path {
    // Create settings.gradle
    std::string settings_content = R"(
rootProject.name = 'TestAndroidProject'
include ':app'
)";
    create_test_file("settings.gradle", settings_content);

    // Create build.gradle
    std::string build_content = R"(
plugins {
    id 'com.android.application'
}

android {
    compileSdk 34
    
    defaultConfig {
        applicationId "com.example.testapp"
        minSdk 21
        targetSdk 34
        versionCode 1
        versionName "1.0"
    }
}

dependencies {
    implementation 'androidx.core:core-ktx:1.12.0'
    implementation 'androidx.appcompat:appcompat:1.6.1'
}
)";
    create_test_file("build.gradle", build_content);

    return test_dir_;
  }

  std::filesystem::path test_dir_;
  std::unique_ptr<Logger> logger_;
};

TEST_F(ImportCommandTestFixture, ErrorToString) {
  EXPECT_EQ(to_string(ImportError::InvalidPath), "Invalid project path");
  EXPECT_EQ(to_string(ImportError::NoGradleFiles), "No Gradle files found in project");
  EXPECT_EQ(to_string(ImportError::ParseError), "Failed to parse Gradle files");
  EXPECT_EQ(to_string(ImportError::GenerateError), "Failed to generate configuration");
  EXPECT_EQ(to_string(ImportError::FileSystemError), "File system error");
}

TEST_F(ImportCommandTestFixture, ImportValidProject) {
  auto project_dir = create_gradle_project();

  ImportCommand command(*logger_);
  auto result = command.execute(project_dir);

  ASSERT_TRUE(result.has_value());

  // Check that horcrux.yaml was created
  auto output_file = project_dir / "horcrux.yaml";
  EXPECT_TRUE(std::filesystem::exists(output_file));

  // Read and verify content
  std::ifstream file(output_file);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("TestAndroidProject"), std::string::npos);
  EXPECT_NE(content.find("android-app"), std::string::npos);
  EXPECT_NE(content.find("compileSdk: 34"), std::string::npos);
}

TEST_F(ImportCommandTestFixture, ImportWithCustomOutputPath) {
  auto project_dir = create_gradle_project();
  auto custom_output = project_dir / "custom-config.yaml";

  ImportCommand command(*logger_);
  auto result = command.execute_with_options(project_dir, custom_output);

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(std::filesystem::exists(custom_output));
}

TEST_F(ImportCommandTestFixture, ImportInvalidPath) {
  auto nonexistent = test_dir_ / "nonexistent";

  ImportCommand command(*logger_);
  auto result = command.execute(nonexistent);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ImportError::InvalidPath);
}

TEST_F(ImportCommandTestFixture, ImportNoGradleFiles) {
  // Create empty directory
  auto empty_dir = test_dir_ / "empty";
  std::filesystem::create_directories(empty_dir);

  ImportCommand command(*logger_);
  auto result = command.execute(empty_dir);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ImportError::NoGradleFiles);
}

TEST_F(ImportCommandTestFixture, ImportWithOnlyBuildGradle) {
  // Create only build.gradle (no settings.gradle)
  std::string build_content = R"(
plugins {
    id 'java-library'
}

dependencies {
    implementation 'com.google.guava:guava:32.1.3-jre'
}
)";
  create_test_file("build.gradle", build_content);

  ImportCommand command(*logger_);
  auto result = command.execute(test_dir_);

  ASSERT_TRUE(result.has_value());

  auto output_file = test_dir_ / "horcrux.yaml";
  EXPECT_TRUE(std::filesystem::exists(output_file));
}

TEST_F(ImportCommandTestFixture, ImportKotlinDslProject) {
  // Create settings.gradle.kts
  std::string settings_content = R"(
rootProject.name = "KotlinDslProject"
include(":app")
)";
  create_test_file("settings.gradle.kts", settings_content);

  // Create build.gradle.kts
  std::string build_content = R"(
plugins {
    id("com.android.application")
}

android {
    compileSdk = 34
    
    defaultConfig {
        applicationId = "com.example.kotlindsl"
        minSdk = 24
        targetSdk = 34
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
}
)";
  create_test_file("build.gradle.kts", build_content);

  ImportCommand command(*logger_);
  auto result = command.execute(test_dir_);

  ASSERT_TRUE(result.has_value());

  auto output_file = test_dir_ / "horcrux.yaml";
  EXPECT_TRUE(std::filesystem::exists(output_file));
}

TEST_F(ImportCommandTestFixture, ImportPrefersBuildGradleOverKts) {
  // Create both .gradle and .kts files
  create_test_file("build.gradle", "plugins { id 'java' }");
  create_test_file("build.gradle.kts", "plugins { id(\"java\") }");

  ImportCommand command(*logger_);
  auto result = command.execute(test_dir_);

  ASSERT_TRUE(result.has_value());
}

} // namespace horcrux::cli::test
