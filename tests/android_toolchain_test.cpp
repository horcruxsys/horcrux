// Horcrux - Android Toolchain Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/core/android_toolchain.h"

namespace horcrux::core::test {

// Helper to create a temporary directory for testing
class TempDir {
public:
  TempDir() {
    path_ = std::filesystem::temp_directory_path() / "horcrux_android_test";
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
TEST(AndroidToolchainErrorTest, ErrorToString) {
  EXPECT_FALSE(to_string(AndroidToolchainError::SdkNotFound).empty());
  EXPECT_FALSE(to_string(AndroidToolchainError::NdkNotFound).empty());
  EXPECT_FALSE(to_string(AndroidToolchainError::JavaNotFound).empty());
  EXPECT_FALSE(to_string(AndroidToolchainError::InvalidSdkStructure).empty());
}

// Test SDK validation
TEST(AndroidToolchainValidatorTest, ValidateSdkReturnsFalseForNonexistent) {
  std::filesystem::path nonexistent = "/nonexistent/path/to/sdk";
  EXPECT_FALSE(AndroidToolchainValidator::validate_sdk(nonexistent));
}

TEST(AndroidToolchainValidatorTest, ValidateSdkReturnsFalseForEmpty) {
  TempDir temp;
  EXPECT_FALSE(AndroidToolchainValidator::validate_sdk(temp.path()));
}

TEST(AndroidToolchainValidatorTest, ValidateSdkReturnsTrueWithPlatforms) {
  TempDir temp;
  std::filesystem::create_directories(temp.path() / "platforms");
  EXPECT_TRUE(AndroidToolchainValidator::validate_sdk(temp.path()));
}

TEST(AndroidToolchainValidatorTest, ValidateSdkReturnsTrueWithBuildTools) {
  TempDir temp;
  std::filesystem::create_directories(temp.path() / "build-tools");
  EXPECT_TRUE(AndroidToolchainValidator::validate_sdk(temp.path()));
}

// Test NDK validation
TEST(AndroidToolchainValidatorTest, ValidateNdkReturnsFalseForNonexistent) {
  std::filesystem::path nonexistent = "/nonexistent/path/to/ndk";
  EXPECT_FALSE(AndroidToolchainValidator::validate_ndk(nonexistent));
}

TEST(AndroidToolchainValidatorTest, ValidateNdkReturnsTrueWithSourceProperties) {
  TempDir temp;

  // Create source.properties file
  std::ofstream props_file(temp.path() / "source.properties");
  props_file << "Pkg.Desc = Android NDK\n";
  props_file << "Pkg.Revision = 25.0.0\n";
  props_file.close();

  EXPECT_TRUE(AndroidToolchainValidator::validate_ndk(temp.path()));
}

// Test Java validation
TEST(AndroidToolchainValidatorTest, ValidateJavaHomeReturnsFalseForNonexistent) {
  std::filesystem::path nonexistent = "/nonexistent/path/to/java";
  EXPECT_FALSE(AndroidToolchainValidator::validate_java_home(nonexistent));
}

TEST(AndroidToolchainValidatorTest, ValidateJavaHomeReturnsTrueWithJavaExecutable) {
  TempDir temp;
  std::filesystem::create_directories(temp.path() / "bin");

#ifdef _WIN32
  std::filesystem::path java_exe = temp.path() / "bin" / "java.exe";
#else
  std::filesystem::path java_exe = temp.path() / "bin" / "java";
#endif

  // Create a dummy file
  std::ofstream file(java_exe);
  file << "#!/bin/bash\n";
  file.close();

  EXPECT_TRUE(AndroidToolchainValidator::validate_java_home(temp.path()));
}

// Test detection with mock SDK - build tools
TEST(AndroidToolchainDetectorTest, DetectFindsBuildTools) {
  TempDir temp;

  // Create build-tools directory structure
  std::filesystem::create_directories(temp.path() / "build-tools" / "34.0.0");
  std::filesystem::create_directories(temp.path() / "build-tools" / "33.0.2");
  std::filesystem::create_directories(temp.path() / "platforms" / "android-33");

  auto result = AndroidToolchainDetector::detect(temp.path());
  ASSERT_TRUE(result.has_value());

  const auto& toolchain = *result;
  EXPECT_EQ(toolchain.build_tools.size(), 2);

  // Should be sorted in descending order
  if (!toolchain.build_tools.empty()) {
    EXPECT_EQ(toolchain.build_tools[0].version, "34.0.0");
  }
}

// Test detection with mock SDK - platforms
TEST(AndroidToolchainDetectorTest, DetectFindsPlatforms) {
  TempDir temp;

  // Create platforms directory structure
  std::filesystem::create_directories(temp.path() / "platforms" / "android-33");
  std::filesystem::create_directories(temp.path() / "platforms" / "android-30");
  std::filesystem::create_directories(temp.path() / "platforms" / "android-28");

  // Create android.jar files
  std::ofstream jar1(temp.path() / "platforms" / "android-33" / "android.jar");
  jar1 << "fake jar";
  jar1.close();

  auto result = AndroidToolchainDetector::detect(temp.path());
  ASSERT_TRUE(result.has_value());

  const auto& toolchain = *result;
  EXPECT_EQ(toolchain.platforms.size(), 3);

  // Should be sorted by API level descending
  if (!toolchain.platforms.empty()) {
    EXPECT_EQ(toolchain.platforms[0].api_level, "33");
  }
}

// Test detection with NDK
TEST(AndroidToolchainDetectorTest, DetectFindsNdkBundle) {
  TempDir temp;
  TempDir ndk_temp;

  // Create minimal SDK
  std::filesystem::create_directories(temp.path() / "platforms" / "android-33");

  // Create NDK bundle structure
  std::ofstream props_file(ndk_temp.path() / "source.properties");
  props_file << "Pkg.Desc = Android NDK\n";
  props_file << "Pkg.Revision = 25.2.9519653\n";
  props_file.close();

  std::filesystem::create_directories(ndk_temp.path() / "toolchains");

  auto result = AndroidToolchainDetector::detect(temp.path(), ndk_temp.path());
  ASSERT_TRUE(result.has_value());

  const auto& toolchain = *result;
  EXPECT_TRUE(toolchain.ndk_root.has_value());
  EXPECT_EQ(toolchain.ndks.size(), 1);

  if (!toolchain.ndks.empty()) {
    EXPECT_EQ(toolchain.ndks[0].version, "25.2.9519653");
  }
}

TEST(AndroidToolchainDetectorTest, DetectFindsVersionedNdks) {
  TempDir temp;
  TempDir ndk_temp;

  // Create minimal SDK
  std::filesystem::create_directories(temp.path() / "platforms");

  // Create versioned NDK structure
  std::filesystem::create_directories(ndk_temp.path() / "25.2.9519653");
  std::filesystem::create_directories(ndk_temp.path() / "26.0.0");

  std::ofstream props1(ndk_temp.path() / "25.2.9519653" / "source.properties");
  props1 << "Pkg.Revision = 25.2.9519653\n";
  props1.close();

  std::ofstream props2(ndk_temp.path() / "26.0.0" / "source.properties");
  props2 << "Pkg.Revision = 26.0.0\n";
  props2.close();

  auto result = AndroidToolchainDetector::detect(temp.path(), ndk_temp.path());
  ASSERT_TRUE(result.has_value());

  const auto& toolchain = *result;
  EXPECT_EQ(toolchain.ndks.size(), 2);
}

// Test full detection with mock SDK
TEST(AndroidToolchainDetectorTest, DetectWithMockSdk) {
  TempDir temp;

  // Create minimal SDK structure
  std::filesystem::create_directories(temp.path() / "build-tools" / "34.0.0");
  std::filesystem::create_directories(temp.path() / "platforms" / "android-33");
  std::filesystem::create_directories(temp.path() / "platform-tools");

  std::ofstream jar(temp.path() / "platforms" / "android-33" / "android.jar");
  jar << "fake jar";
  jar.close();

  auto result = AndroidToolchainDetector::detect(temp.path());
  ASSERT_TRUE(result.has_value());

  const auto& toolchain = *result;
  EXPECT_EQ(toolchain.sdk_root, temp.path());
  EXPECT_FALSE(toolchain.build_tools.empty());
  EXPECT_FALSE(toolchain.platforms.empty());
  EXPECT_TRUE(toolchain.platform_tools.has_value());
}

// Test JSON serialization
TEST(AndroidToolchainTest, ToJsonProducesValidJson) {
  TempDir temp;

  AndroidToolchain toolchain;
  toolchain.sdk_root = temp.path();
  toolchain.merkle_hash = "abc123";

  AndroidBuildTools bt;
  bt.version = "34.0.0";
  bt.path = temp.path() / "build-tools" / "34.0.0";
  toolchain.build_tools.push_back(bt);

  AndroidPlatform platform;
  platform.api_level = "33";
  platform.version = "13.0";
  platform.path = temp.path() / "platforms" / "android-33";
  toolchain.platforms.push_back(platform);

  std::string json = toolchain.to_json();

  // Basic validation - check for key fields
  EXPECT_NE(json.find("sdk_root"), std::string::npos);
  EXPECT_NE(json.find("build_tools"), std::string::npos);
  EXPECT_NE(json.find("platforms"), std::string::npos);
  EXPECT_NE(json.find("merkle_hash"), std::string::npos);
  EXPECT_NE(json.find("abc123"), std::string::npos);
}

// Test validation
TEST(AndroidToolchainDetectorTest, ValidateRejectsInvalidSdk) {
  AndroidToolchain toolchain;
  toolchain.sdk_root = "/nonexistent/path";
  toolchain.merkle_hash = "test";

  auto result = AndroidToolchainDetector::validate(toolchain);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidToolchainError::InvalidSdkStructure);
}

TEST(AndroidToolchainDetectorTest, ValidateAcceptsValidSdk) {
  TempDir temp;

  // Create minimal valid SDK
  std::filesystem::create_directories(temp.path() / "platforms");

  AndroidToolchain toolchain;
  toolchain.sdk_root = temp.path();
  toolchain.merkle_hash = "test";

  auto result = AndroidToolchainDetector::validate(toolchain);
  EXPECT_TRUE(result.has_value());
}

} // namespace horcrux::core::test
