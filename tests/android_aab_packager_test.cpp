// Horcrux - Android AAB Packager Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "android_aab_packager.h"
#include "android_toolchain.h"

using namespace horcrux::core;

namespace {

// Test fixture for AAB packager tests
class AabPackagerTestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    // Create temporary test directory
    test_dir_ = std::filesystem::temp_directory_path() / "horcrux_aab_test";
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

    // Create mock platform
    AndroidPlatform platform;
    platform.api_level = "34";
    platform.version = "14.0";
    platform.path = toolchain.sdk_root / "platforms" / "android-34";
    std::filesystem::create_directories(platform.path);
    
    toolchain.platforms.push_back(platform);
    return toolchain;
  }

  auto create_mock_manifest() -> std::filesystem::path {
    auto manifest = test_dir_ / "AndroidManifest.xml";
    std::ofstream file(manifest);
    file << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    file << "<manifest package=\"com.example.test\">\n";
    file << "  <application>\n";
    file << "  </application>\n";
    file << "</manifest>\n";
    file.close();
    return manifest;
  }

  auto create_mock_resources_apk() -> std::filesystem::path {
    auto resources_apk = test_dir_ / "resources.ap_";
    
    // Create a mock ZIP file
    auto temp_content = test_dir_ / "temp_resources";
    std::filesystem::create_directories(temp_content);
    
    // Create mock res directory
    auto res_dir = temp_content / "res";
    std::filesystem::create_directories(res_dir);
    
    auto values_dir = res_dir / "values";
    std::filesystem::create_directories(values_dir);
    
    auto strings_xml = values_dir / "strings.xml";
    std::ofstream strings_file(strings_xml);
    strings_file << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    strings_file << "<resources>\n";
    strings_file << "  <string name=\"app_name\">Test App</string>\n";
    strings_file << "</resources>\n";
    strings_file.close();

    // Create ZIP
    std::string cmd = "cd " + temp_content.string() + " && zip -q -r " +
                     resources_apk.string() + " .";
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
TEST_F(AabPackagerTestFixture, ErrorToString) {
  EXPECT_EQ(to_string(AndroidAabPackagerError::InvalidConfiguration),
            "Invalid configuration");
  EXPECT_EQ(to_string(AndroidAabPackagerError::BundletoolNotFound),
            "bundletool not found");
  EXPECT_EQ(to_string(AndroidAabPackagerError::PackagingFailed),
            "AAB packaging failed");
  EXPECT_EQ(to_string(AndroidAabPackagerError::ModuleCreationFailed),
            "Module creation failed");
  EXPECT_EQ(to_string(AndroidAabPackagerError::UniversalApkFailed),
            "Universal APK generation failed");
  EXPECT_EQ(to_string(AndroidAabPackagerError::IoError), "I/O error");
  EXPECT_EQ(to_string(AndroidAabPackagerError::UnknownError), "Unknown error");
}

// Test packager construction
TEST_F(AabPackagerTestFixture, PackagerConstruction) {
  AndroidAabPackager packager(mock_toolchain_);
  
  // bundletool may or may not exist in test environment
  auto bundletool_path = packager.get_bundletool_path();
  // Just check that the method runs without error
  SUCCEED();
}

// Test validate config with no modules
TEST_F(AabPackagerTestFixture, ValidateConfigNoModules) {
  AabPackagingConfig config;
  config.output_aab = test_dir_ / "output.aab";

  auto result = AndroidAabPackager::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidAabPackagerError::InvalidConfiguration);
}

// Test validate config with no base module
TEST_F(AabPackagerTestFixture, ValidateConfigNoBaseModule) {
  AabPackagingConfig config;
  config.output_aab = test_dir_ / "output.aab";

  AabModuleConfig module;
  module.module_name = "feature1";
  module.manifest = create_mock_manifest();
  module.resources_apk = create_mock_resources_apk();
  module.is_base_module = false;

  config.modules.push_back(module);

  auto result = AndroidAabPackager::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidAabPackagerError::InvalidConfiguration);
}

// Test validate config with multiple base modules
TEST_F(AabPackagerTestFixture, ValidateConfigMultipleBaseModules) {
  AabPackagingConfig config;
  config.output_aab = test_dir_ / "output.aab";

  AabModuleConfig base1;
  base1.module_name = "base";
  base1.manifest = create_mock_manifest();
  base1.resources_apk = create_mock_resources_apk();
  base1.is_base_module = true;

  AabModuleConfig base2;
  base2.module_name = "base2";
  base2.manifest = create_mock_manifest();
  base2.resources_apk = create_mock_resources_apk();
  base2.is_base_module = true;

  config.modules.push_back(base1);
  config.modules.push_back(base2);

  auto result = AndroidAabPackager::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidAabPackagerError::InvalidConfiguration);
}

// Test validate config with valid single module
TEST_F(AabPackagerTestFixture, ValidateConfigSuccess) {
  AabPackagingConfig config;
  config.output_aab = test_dir_ / "output.aab";

  AabModuleConfig base;
  base.module_name = "base";
  base.manifest = create_mock_manifest();
  base.resources_apk = create_mock_resources_apk();
  base.is_base_module = true;

  config.modules.push_back(base);

  auto result = AndroidAabPackager::validate_config(config);
  EXPECT_TRUE(result.has_value());
}

// Test validate config with base and feature modules
TEST_F(AabPackagerTestFixture, ValidateConfigWithFeatureModules) {
  AabPackagingConfig config;
  config.output_aab = test_dir_ / "output.aab";

  AabModuleConfig base;
  base.module_name = "base";
  base.manifest = create_mock_manifest();
  base.resources_apk = create_mock_resources_apk();
  base.is_base_module = true;

  AabModuleConfig feature1;
  feature1.module_name = "feature1";
  feature1.manifest = create_mock_manifest();
  feature1.resources_apk = create_mock_resources_apk();
  feature1.is_base_module = false;

  AabModuleConfig feature2;
  feature2.module_name = "feature2";
  feature2.manifest = create_mock_manifest();
  feature2.resources_apk = create_mock_resources_apk();
  feature2.is_base_module = false;

  config.modules.push_back(base);
  config.modules.push_back(feature1);
  config.modules.push_back(feature2);

  auto result = AndroidAabPackager::validate_config(config);
  EXPECT_TRUE(result.has_value());
}

// Test compute packaging hash
TEST_F(AabPackagerTestFixture, ComputePackagingHash) {
  AabPackagingConfig config;
  config.output_aab = test_dir_ / "output.aab";

  AabModuleConfig base;
  base.module_name = "base";
  base.manifest = test_dir_ / "AndroidManifest.xml";
  base.resources_apk = test_dir_ / "resources.ap_";
  base.is_base_module = true;

  config.modules.push_back(base);

  auto hash1 = AndroidAabPackager::compute_packaging_hash(config);
  EXPECT_FALSE(hash1.empty());
  EXPECT_EQ(hash1.length(), 64); // SHA-256 produces 64 hex characters

  // Same config should produce same hash
  auto hash2 = AndroidAabPackager::compute_packaging_hash(config);
  EXPECT_EQ(hash1, hash2);

  // Different config should produce different hash
  AabModuleConfig feature;
  feature.module_name = "feature1";
  feature.manifest = test_dir_ / "feature_manifest.xml";
  feature.resources_apk = test_dir_ / "feature_resources.ap_";
  feature.is_base_module = false;
  config.modules.push_back(feature);

  auto hash3 = AndroidAabPackager::compute_packaging_hash(config);
  EXPECT_NE(hash1, hash3);
}

// Test AAB info extraction
TEST_F(AabPackagerTestFixture, GetAabInfo) {
  auto aab_path = test_dir_ / "test.aab";
  
  auto result = aab_utils::get_aab_info(aab_path);
  EXPECT_TRUE(result.has_value());
  
  if (result) {
    EXPECT_FALSE(result->module_names.empty());
    EXPECT_FALSE(result->package_name.empty());
  }
}

// Test AAB validation
TEST_F(AabPackagerTestFixture, ValidateAab) {
  auto aab_path = test_dir_ / "nonexistent.aab";
  
  auto result = aab_utils::validate_aab(aab_path);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidAabPackagerError::InvalidConfiguration);
}

// Test packaging hash determinism
TEST_F(AabPackagerTestFixture, PackagingHashDeterminism) {
  AabPackagingConfig config;
  config.output_aab = test_dir_ / "output.aab";

  AabModuleConfig base;
  base.module_name = "base";
  base.manifest = test_dir_ / "AndroidManifest.xml";
  base.resources_apk = test_dir_ / "resources.ap_";
  base.dex_files = {test_dir_ / "classes.dex"};
  base.is_base_module = true;

  config.modules.push_back(base);

  // Compute hash multiple times
  auto hash1 = AndroidAabPackager::compute_packaging_hash(config);
  auto hash2 = AndroidAabPackager::compute_packaging_hash(config);
  auto hash3 = AndroidAabPackager::compute_packaging_hash(config);

  // All hashes should be identical
  EXPECT_EQ(hash1, hash2);
  EXPECT_EQ(hash2, hash3);
}

// Test module configuration
TEST_F(AabPackagerTestFixture, ModuleConfiguration) {
  AabModuleConfig module;
  module.module_name = "base";
  module.manifest = create_mock_manifest();
  module.resources_apk = create_mock_resources_apk();
  module.dex_files.push_back(create_mock_dex_file());
  module.is_base_module = true;

  EXPECT_EQ(module.module_name, "base");
  EXPECT_TRUE(module.is_base_module);
  EXPECT_TRUE(std::filesystem::exists(module.manifest));
  EXPECT_TRUE(std::filesystem::exists(module.resources_apk));
  EXPECT_EQ(module.dex_files.size(), 1);
}

// Test universal APK config validation
TEST_F(AabPackagerTestFixture, UniversalApkConfigValidation) {
  UniversalApkConfig config;
  config.aab_path = test_dir_ / "nonexistent.aab";
  config.output_apk = test_dir_ / "universal.apk";

  AndroidAabPackager packager(mock_toolchain_);
  
  auto result = packager.generate_universal_apk(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidAabPackagerError::InvalidConfiguration);
}

// Test empty module name
TEST_F(AabPackagerTestFixture, EmptyModuleName) {
  AabPackagingConfig config;
  config.output_aab = test_dir_ / "output.aab";

  AabModuleConfig base;
  base.module_name = ""; // Empty name
  base.manifest = create_mock_manifest();
  base.resources_apk = create_mock_resources_apk();
  base.is_base_module = true;

  config.modules.push_back(base);

  // Module with empty name should still validate (name validation is module-specific)
  auto result = AndroidAabPackager::validate_config(config);
  EXPECT_TRUE(result.has_value());
}

} // anonymous namespace
