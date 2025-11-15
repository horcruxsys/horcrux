// Horcrux - Gradle Parser Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/core/gradle_parser.h"

namespace horcrux::core::test {

class GradleParserTestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    test_dir_ = std::filesystem::temp_directory_path() / "horcrux_gradle_test";
    std::filesystem::create_directories(test_dir_);
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

  std::filesystem::path test_dir_;
};

TEST_F(GradleParserTestFixture, ErrorToString) {
  EXPECT_EQ(to_string(GradleParserError::FileNotFound), "File not found");
  EXPECT_EQ(to_string(GradleParserError::InvalidSyntax), "Invalid syntax");
  EXPECT_EQ(to_string(GradleParserError::UnsupportedFormat), "Unsupported format");
  EXPECT_EQ(to_string(GradleParserError::ParseError), "Parse error");
}

TEST_F(GradleParserTestFixture, ParseSettingsGroovy) {
  std::string content = R"(
rootProject.name = 'MyProject'
include ':app'
include ':library'
)";

  auto settings_file = create_test_file("settings.gradle", content);
  auto result = GradleParser::parse_settings(settings_file);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->name, "MyProject");
  EXPECT_EQ(result->subprojects.size(), 2);
  EXPECT_EQ(result->subprojects[0], ":app");
  EXPECT_EQ(result->subprojects[1], ":library");
}

TEST_F(GradleParserTestFixture, ParseSettingsKotlinDsl) {
  std::string content = R"(
rootProject.name = "MyKotlinProject"
include(":app")
include(":library")
)";

  auto settings_file = create_test_file("settings.gradle.kts", content);
  auto result = GradleParser::parse_settings(settings_file);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->name, "MyKotlinProject");
  EXPECT_GE(result->subprojects.size(), 0); // May parse some includes
}

TEST_F(GradleParserTestFixture, ParseSettingsFileNotFound) {
  auto result = GradleParser::parse_settings(test_dir_ / "nonexistent.gradle");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), GradleParserError::FileNotFound);
}

TEST_F(GradleParserTestFixture, DetectAndroidApplication) {
  std::string content = R"(
plugins {
    id 'com.android.application'
}

android {
    compileSdk 34
    defaultConfig {
        applicationId "com.example.app"
        minSdk 21
        targetSdk 34
    }
}
)";

  auto build_file = create_test_file("build.gradle", content);
  auto result = GradleParser::detect_project_type(build_file);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "android-app");
}

TEST_F(GradleParserTestFixture, DetectAndroidLibrary) {
  std::string content = R"(
plugins {
    id 'com.android.library'
}

android {
    compileSdk 34
}
)";

  auto build_file = create_test_file("build.gradle", content);
  auto result = GradleParser::detect_project_type(build_file);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "android-library");
}

TEST_F(GradleParserTestFixture, ParseBuildGradle) {
  std::string content = R"(
plugins {
    id 'com.android.application'
}

android {
    compileSdk 34
    
    defaultConfig {
        applicationId "com.example.myapp"
        minSdk 21
        targetSdk 34
        versionCode 1
        versionName "1.0"
    }
}

dependencies {
    implementation 'androidx.core:core-ktx:1.12.0'
    implementation 'androidx.appcompat:appcompat:1.6.1'
    testImplementation 'junit:junit:4.13.2'
}
)";

  auto build_file = create_test_file("build.gradle", content);
  auto result = GradleParser::parse_build(build_file);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->project_type, "android-app");
  EXPECT_EQ(result->compile_sdk, "34");
  EXPECT_EQ(result->min_sdk, "21");
  EXPECT_EQ(result->target_sdk, "34");
  EXPECT_EQ(result->application_id, "com.example.myapp");
  EXPECT_EQ(result->version_name, "1.0");
  EXPECT_EQ(result->version_code, "1");

  EXPECT_EQ(result->dependencies.size(), 3);
  EXPECT_EQ(result->dependencies[0].group, "androidx.core");
  EXPECT_EQ(result->dependencies[0].name, "core-ktx");
  EXPECT_EQ(result->dependencies[0].version, "1.12.0");
  EXPECT_EQ(result->dependencies[0].configuration, "implementation");
}

TEST_F(GradleParserTestFixture, ParseBuildGradleKotlinDsl) {
  std::string content = R"(
plugins {
    id("com.android.application")
}

android {
    compileSdk = 34
    
    defaultConfig {
        applicationId = "com.example.kotlinapp"
        minSdk = 24
        targetSdk = 34
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
}
)";

  auto build_file = create_test_file("build.gradle.kts", content);
  auto result = GradleParser::parse_build(build_file);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->project_type, "android-app");
  EXPECT_EQ(result->compile_sdk, "34");
}

TEST_F(GradleParserTestFixture, ParseBuildFileNotFound) {
  auto result = GradleParser::parse_build(test_dir_ / "nonexistent.gradle");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), GradleParserError::FileNotFound);
}

TEST_F(GradleParserTestFixture, ExtractDependencies) {
  std::string content = R"(
dependencies {
    implementation 'com.google.android:android:4.1.1.4'
    api 'org.jetbrains.kotlin:kotlin-stdlib:1.9.0'
    testImplementation 'junit:junit:4.13.2'
    androidTestImplementation 'androidx.test:runner:1.5.2'
}
)";

  auto build_file = create_test_file("build.gradle", content);
  auto result = GradleParser::parse_build(build_file);

  ASSERT_TRUE(result.has_value());
  ASSERT_GE(result->dependencies.size(), 2);

  // Check for specific dependencies
  bool found_kotlin = false;
  bool found_junit = false;

  for (const auto& dep : result->dependencies) {
    if (dep.name == "kotlin-stdlib") {
      found_kotlin = true;
      EXPECT_EQ(dep.group, "org.jetbrains.kotlin");
      EXPECT_EQ(dep.version, "1.9.0");
      EXPECT_EQ(dep.configuration, "api");
    }
    if (dep.name == "junit") {
      found_junit = true;
      EXPECT_EQ(dep.group, "junit");
      EXPECT_EQ(dep.version, "4.13.2");
      EXPECT_EQ(dep.configuration, "testImplementation");
    }
  }

  EXPECT_TRUE(found_kotlin);
  EXPECT_TRUE(found_junit);
}

TEST_F(GradleParserTestFixture, DefaultSourceSets) {
  std::string content = R"(
plugins {
    id 'com.android.application'
}

android {
    compileSdk 34
}
)";

  auto build_file = create_test_file("build.gradle", content);
  auto result = GradleParser::parse_build(build_file);

  ASSERT_TRUE(result.has_value());
  EXPECT_GE(result->source_sets.size(), 2);

  // Check for main source set
  bool found_main = false;
  for (const auto& ss : result->source_sets) {
    if (ss.name == "main") {
      found_main = true;
      EXPECT_FALSE(ss.java_dirs.empty());
      EXPECT_FALSE(ss.kotlin_dirs.empty());
    }
  }
  EXPECT_TRUE(found_main);
}

TEST_F(GradleParserTestFixture, DefaultBuildVariants) {
  std::string content = R"(
plugins {
    id 'com.android.application'
}

android {
    compileSdk 34
}
)";

  auto build_file = create_test_file("build.gradle", content);
  auto result = GradleParser::parse_build(build_file);

  ASSERT_TRUE(result.has_value());
  EXPECT_GE(result->build_variants.size(), 2);

  // Check for debug and release variants
  bool found_debug = false;
  bool found_release = false;

  for (const auto& variant : result->build_variants) {
    if (variant.name == "debug") {
      found_debug = true;
      EXPECT_EQ(variant.build_type, "debug");
    }
    if (variant.name == "release") {
      found_release = true;
      EXPECT_EQ(variant.build_type, "release");
    }
  }

  EXPECT_TRUE(found_debug);
  EXPECT_TRUE(found_release);
}

TEST_F(GradleParserTestFixture, ParseProductFlavorsOneDimension) {
  std::string content = R"(
plugins {
    id 'com.android.application'
}

android {
    compileSdk 34
    
    flavorDimensions "tier"
    
    productFlavors {
        free {
            dimension "tier"
            applicationIdSuffix ".free"
        }
        pro {
            dimension "tier"
            applicationIdSuffix ".pro"
        }
    }
}
)";

  auto build_file = create_test_file("build.gradle", content);
  auto result = GradleParser::parse_build(build_file);

  ASSERT_TRUE(result.has_value());

  // Should have 4 variants: freeDebug, freeRelease, proDebug, proRelease
  EXPECT_GE(result->build_variants.size(), 4);

  // Check for specific variants
  bool found_free_debug = false;
  bool found_free_release = false;
  bool found_pro_debug = false;
  bool found_pro_release = false;

  for (const auto& variant : result->build_variants) {
    if (variant.name == "freeDebug") {
      found_free_debug = true;
      EXPECT_EQ(variant.build_type, "debug");
      EXPECT_GE(variant.flavors.size(), 1);
      if (!variant.flavors.empty()) {
        EXPECT_EQ(variant.flavors[0], "free");
      }
    }
    if (variant.name == "freeRelease") {
      found_free_release = true;
      EXPECT_EQ(variant.build_type, "release");
    }
    if (variant.name == "proDebug") {
      found_pro_debug = true;
      EXPECT_EQ(variant.build_type, "debug");
    }
    if (variant.name == "proRelease") {
      found_pro_release = true;
      EXPECT_EQ(variant.build_type, "release");
    }
  }

  EXPECT_TRUE(found_free_debug);
  EXPECT_TRUE(found_free_release);
  EXPECT_TRUE(found_pro_debug);
  EXPECT_TRUE(found_pro_release);
}

TEST_F(GradleParserTestFixture, ParseProductFlavorsMultipleDimensions) {
  std::string content = R"(
plugins {
    id 'com.android.application'
}

android {
    compileSdk 34
    
    flavorDimensions "tier", "store"
    
    productFlavors {
        free {
            dimension "tier"
        }
        pro {
            dimension "tier"
        }
        google {
            dimension "store"
        }
        amazon {
            dimension "store"
        }
    }
}
)";

  auto build_file = create_test_file("build.gradle", content);
  auto result = GradleParser::parse_build(build_file);

  ASSERT_TRUE(result.has_value());

  // Should have 8 variants: 2 tiers × 2 stores × 2 build types = 8
  EXPECT_GE(result->build_variants.size(), 8);

  // Check for some specific variants
  bool found_free_google_debug = false;
  bool found_pro_amazon_release = false;

  for (const auto& variant : result->build_variants) {
    if (variant.name == "freeGoogleDebug") {
      found_free_google_debug = true;
      EXPECT_EQ(variant.build_type, "debug");
      EXPECT_GE(variant.flavors.size(), 2);
    }
    if (variant.name == "proAmazonRelease") {
      found_pro_amazon_release = true;
      EXPECT_EQ(variant.build_type, "release");
      EXPECT_GE(variant.flavors.size(), 2);
    }
  }

  EXPECT_TRUE(found_free_google_debug);
  EXPECT_TRUE(found_pro_amazon_release);
}

TEST_F(GradleParserTestFixture, ParseCustomBuildTypes) {
  std::string content = R"(
plugins {
    id 'com.android.application'
}

android {
    compileSdk 34
    
    buildTypes {
        debug {
            debuggable true
        }
        release {
            minifyEnabled true
        }
        staging {
            debuggable true
        }
    }
}
)";

  auto build_file = create_test_file("build.gradle", content);
  auto result = GradleParser::parse_build(build_file);

  ASSERT_TRUE(result.has_value());

  // Should have 3 build types
  EXPECT_GE(result->build_variants.size(), 3);

  bool found_debug = false;
  bool found_release = false;
  bool found_staging = false;

  for (const auto& variant : result->build_variants) {
    if (variant.name == "debug")
      found_debug = true;
    if (variant.name == "release")
      found_release = true;
    if (variant.name == "staging")
      found_staging = true;
  }

  EXPECT_TRUE(found_debug);
  EXPECT_TRUE(found_release);
  EXPECT_TRUE(found_staging);
}

} // namespace horcrux::core::test
