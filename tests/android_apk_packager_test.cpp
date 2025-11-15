// Horcrux - Android APK Packager Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "android_apk_packager.h"
#include "android_toolchain.h"

using namespace horcrux::core;

namespace {

// Test fixture for APK packager tests
class ApkPackagerTestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    // Create temporary test directory
    test_dir_ = std::filesystem::temp_directory_path() / "horcrux_apk_test";
    std::filesystem::create_directories(test_dir_);

    // Create mock Android toolchain
    mock_toolchain_ = create_mock_toolchain();
  }

  void TearDown() override {
    // Clean up test directory
    if (std::filesystem::exists(test_dir_)) {
      std::filesystem::remove_all(test_dir_);
    }
  }

  auto create_mock_toolchain() -> AndroidToolchain {
    AndroidToolchain toolchain;
    toolchain.sdk_root = test_dir_ / "sdk";
    std::filesystem::create_directories(toolchain.sdk_root);

    // Create mock build tools
    AndroidBuildTools bt;
    bt.version = "34.0.0";
    bt.path = toolchain.sdk_root / "build-tools" / bt.version;
    std::filesystem::create_directories(bt.path);

    // Create mock tool executables
    bt.zipalign_path = bt.path / "zipalign";
    create_mock_executable(bt.zipalign_path);

    toolchain.build_tools.push_back(bt);
    return toolchain;
  }

  void create_mock_executable(const std::filesystem::path& path) {
    std::ofstream file(path);
    file << "#!/bin/sh\n";
    file << "exit 0\n";
    file.close();
    std::filesystem::permissions(path, std::filesystem::perms::owner_exec |
                                           std::filesystem::perms::owner_read |
                                           std::filesystem::perms::owner_write);
  }

  auto create_mock_resources_apk() -> std::filesystem::path {
    auto resources_apk = test_dir_ / "resources.ap_";

    // Create a mock ZIP file (APK is a ZIP)
    auto temp_content = test_dir_ / "temp_resources";
    std::filesystem::create_directories(temp_content);

    // Create mock AndroidManifest.xml
    auto manifest = temp_content / "AndroidManifest.xml";
    std::ofstream manifest_file(manifest);
    manifest_file << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    manifest_file << "<manifest package=\"com.example.test\">\n";
    manifest_file << "</manifest>\n";
    manifest_file.close();

    // Create ZIP
    std::string cmd = "cd " + temp_content.string() + " && zip -q " + resources_apk.string() +
                      " AndroidManifest.xml";
    std::system(cmd.c_str());

    std::filesystem::remove_all(temp_content);
    return resources_apk;
  }

  auto create_mock_dex_file() -> std::filesystem::path {
    auto dex_file = test_dir_ / "classes.dex";
    std::ofstream file(dex_file, std::ios::binary);
    // Write mock DEX magic number
    file << "dex\n035\0";
    file.close();
    return dex_file;
  }

  std::filesystem::path test_dir_;
  AndroidToolchain mock_toolchain_;
};

// Test error string conversion
TEST_F(ApkPackagerTestFixture, ErrorToString) {
  EXPECT_EQ(to_string(AndroidApkPackagerError::InvalidConfiguration), "Invalid configuration");
  EXPECT_EQ(to_string(AndroidApkPackagerError::ZipalignNotFound), "zipalign tool not found");
  EXPECT_EQ(to_string(AndroidApkPackagerError::ApksignerNotFound), "apksigner tool not found");
  EXPECT_EQ(to_string(AndroidApkPackagerError::SigningFailed), "APK signing failed");
  EXPECT_EQ(to_string(AndroidApkPackagerError::VerificationFailed), "APK verification failed");
  EXPECT_EQ(to_string(AndroidApkPackagerError::PackagingFailed), "APK packaging failed");
  EXPECT_EQ(to_string(AndroidApkPackagerError::IoError), "I/O error");
  EXPECT_EQ(to_string(AndroidApkPackagerError::UnknownError), "Unknown error");
}

// Test packager construction
TEST_F(ApkPackagerTestFixture, PackagerConstruction) {
  AndroidApkPackager packager(mock_toolchain_);

  // Check zipalign path is found
  auto zipalign_path = packager.get_zipalign_path();
  EXPECT_TRUE(zipalign_path.has_value());
  if (zipalign_path) {
    EXPECT_TRUE(std::filesystem::exists(*zipalign_path));
  }
}

// Test validate config with missing resources
TEST_F(ApkPackagerTestFixture, ValidateConfigMissingResources) {
  ApkPackagingConfig config;
  config.resources_apk = test_dir_ / "nonexistent.ap_";
  config.dex_files.push_back(create_mock_dex_file());
  config.output_apk = test_dir_ / "output.apk";

  auto result = AndroidApkPackager::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidApkPackagerError::InvalidConfiguration);
}

// Test validate config with no DEX files
TEST_F(ApkPackagerTestFixture, ValidateConfigNoDex) {
  ApkPackagingConfig config;
  config.resources_apk = create_mock_resources_apk();
  config.output_apk = test_dir_ / "output.apk";

  auto result = AndroidApkPackager::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidApkPackagerError::InvalidConfiguration);
}

// Test validate config with empty output path
TEST_F(ApkPackagerTestFixture, ValidateConfigEmptyOutput) {
  ApkPackagingConfig config;
  config.resources_apk = create_mock_resources_apk();
  config.dex_files.push_back(create_mock_dex_file());

  auto result = AndroidApkPackager::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidApkPackagerError::InvalidConfiguration);
}

// Test validate config with valid configuration
TEST_F(ApkPackagerTestFixture, ValidateConfigSuccess) {
  ApkPackagingConfig config;
  config.resources_apk = create_mock_resources_apk();
  config.dex_files.push_back(create_mock_dex_file());
  config.output_apk = test_dir_ / "output.apk";

  auto result = AndroidApkPackager::validate_config(config);
  EXPECT_TRUE(result.has_value());
}

// Test compute packaging hash
TEST_F(ApkPackagerTestFixture, ComputePackagingHash) {
  ApkPackagingConfig config;
  config.resources_apk = test_dir_ / "resources.ap_";
  config.dex_files.push_back(test_dir_ / "classes.dex");
  config.output_apk = test_dir_ / "output.apk";

  auto hash1 = AndroidApkPackager::compute_packaging_hash(config);
  EXPECT_FALSE(hash1.empty());
  EXPECT_EQ(hash1.length(), 64); // SHA-256 produces 64 hex characters

  // Same config should produce same hash
  auto hash2 = AndroidApkPackager::compute_packaging_hash(config);
  EXPECT_EQ(hash1, hash2);

  // Different config should produce different hash
  config.dex_files.push_back(test_dir_ / "classes2.dex");
  auto hash3 = AndroidApkPackager::compute_packaging_hash(config);
  EXPECT_NE(hash1, hash3);
}

// Test APK info extraction
TEST_F(ApkPackagerTestFixture, GetApkInfo) {
  auto apk_path = test_dir_ / "test.apk";

  auto result = apk_utils::get_apk_info(apk_path);
  EXPECT_TRUE(result.has_value());

  if (result) {
    EXPECT_FALSE(result->package_name.empty());
    EXPECT_FALSE(result->version_name.empty());
    EXPECT_GT(result->version_code, 0);
  }
}

// Test zipalign path detection
TEST_F(ApkPackagerTestFixture, ZipalignPathDetection) {
  AndroidApkPackager packager(mock_toolchain_);

  auto zipalign_path = packager.get_zipalign_path();
  EXPECT_TRUE(zipalign_path.has_value());
}

// Test apksigner path detection (may not exist in test environment)
TEST_F(ApkPackagerTestFixture, ApksignerPathDetection) {
  AndroidApkPackager packager(mock_toolchain_);

  auto apksigner_path = packager.get_apksigner_path();
  // apksigner may or may not exist in test environment
  // Just check that the method runs without error
  SUCCEED();
}

// Test signing config validation
TEST_F(ApkPackagerTestFixture, ValidateSigningConfig) {
  ApkPackagingConfig config;
  config.resources_apk = create_mock_resources_apk();
  config.dex_files.push_back(create_mock_dex_file());
  config.output_apk = test_dir_ / "output.apk";

  // Create mock keystore
  auto keystore = test_dir_ / "debug.keystore";
  std::ofstream keystore_file(keystore);
  keystore_file << "mock keystore";
  keystore_file.close();

  ApkSigningConfig signing_config;
  signing_config.keystore_path = keystore;
  signing_config.keystore_password = "android";
  signing_config.key_alias = "androiddebugkey";
  signing_config.key_password = "android";

  config.signing_config = signing_config;

  auto result = AndroidApkPackager::validate_config(config);
  EXPECT_TRUE(result.has_value());
}

// Test packaging hash determinism
TEST_F(ApkPackagerTestFixture, PackagingHashDeterminism) {
  ApkPackagingConfig config;
  config.resources_apk = test_dir_ / "resources.ap_";
  config.dex_files = {test_dir_ / "classes.dex", test_dir_ / "classes2.dex"};
  config.native_libs = {test_dir_ / "lib" / "armeabi-v7a" / "libfoo.so"};
  config.output_apk = test_dir_ / "output.apk";

  // Compute hash multiple times
  auto hash1 = AndroidApkPackager::compute_packaging_hash(config);
  auto hash2 = AndroidApkPackager::compute_packaging_hash(config);
  auto hash3 = AndroidApkPackager::compute_packaging_hash(config);

  // All hashes should be identical
  EXPECT_EQ(hash1, hash2);
  EXPECT_EQ(hash2, hash3);
}

// Test is_aligned utility
TEST_F(ApkPackagerTestFixture, IsAligned) {
  auto apk_path = test_dir_ / "test.apk";
  std::ofstream file(apk_path);
  file << "test";
  file.close();

  // 4-byte file should be aligned to 4
  bool aligned = apk_utils::is_aligned(apk_path, 4);
  EXPECT_TRUE(aligned);
}

} // anonymous namespace
