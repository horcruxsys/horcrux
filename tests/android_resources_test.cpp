// Horcrux - Android Resource Processing Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "android_resources.h"
#include "android_toolchain.h"

using namespace horcrux::core;

// Helper function to create temporary test files
class ResourceTestFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create temporary test directory
    test_dir_ = std::filesystem::temp_directory_path() / "horcrux_resource_test";
    std::filesystem::create_directories(test_dir_);
  }

  void TearDown() override {
    // Clean up test directory
    if (std::filesystem::exists(test_dir_)) {
      std::filesystem::remove_all(test_dir_);
    }
  }

  std::filesystem::path test_dir_;

  // Helper to create a test resource file
  void create_test_resource(const std::string& subpath, const std::string& content) {
    auto path = test_dir_ / subpath;
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    file << content;
  }

  // Helper to create a simple test manifest
  void create_test_manifest(const std::filesystem::path& path) {
    std::ofstream file(path);
    file << R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.test"
    android:versionCode="1"
    android:versionName="1.0">
    <uses-sdk
        android:minSdkVersion="21"
        android:targetSdkVersion="34" />
</manifest>)";
  }
};

// Test error to string conversion
TEST(AndroidResourceErrorTest, ErrorToString) {
  EXPECT_EQ(to_string(AndroidResourceError::Aapt2NotFound),
            "AAPT2 not found in Android build tools");
  EXPECT_EQ(to_string(AndroidResourceError::InvalidResourceFile),
            "Invalid resource file");
  EXPECT_EQ(to_string(AndroidResourceError::CompilationFailed),
            "Resource compilation failed");
  EXPECT_EQ(to_string(AndroidResourceError::LinkingFailed),
            "Resource linking failed");
}

// Test ResourceType conversion
TEST(ResourceUtilsTest, ResourceTypeConversion) {
  EXPECT_EQ(resource_utils::resource_type_to_string(ResourceType::Values), "values");
  EXPECT_EQ(resource_utils::resource_type_to_string(ResourceType::Layout), "layout");
  EXPECT_EQ(resource_utils::resource_type_to_string(ResourceType::Drawable), "drawable");
  
  EXPECT_EQ(resource_utils::string_to_resource_type("values"), ResourceType::Values);
  EXPECT_EQ(resource_utils::string_to_resource_type("layout"), ResourceType::Layout);
  EXPECT_EQ(resource_utils::string_to_resource_type("unknown"), ResourceType::Unknown);
}

// Test ResourceDensity conversion
TEST(ResourceUtilsTest, DensityConversion) {
  EXPECT_EQ(resource_utils::density_to_string(ResourceDensity::MDPI), "mdpi");
  EXPECT_EQ(resource_utils::density_to_string(ResourceDensity::HDPI), "hdpi");
  EXPECT_EQ(resource_utils::density_to_string(ResourceDensity::XHDPI), "xhdpi");
  
  EXPECT_EQ(resource_utils::string_to_density("mdpi"), ResourceDensity::MDPI);
  EXPECT_EQ(resource_utils::string_to_density("hdpi"), ResourceDensity::HDPI);
  EXPECT_EQ(resource_utils::string_to_density("unknown"), ResourceDensity::None);
}

// Test ResourceQualifiers parsing
TEST(ResourceQualifiersTest, ParseSimpleQualifiers) {
  auto qualifiers = ResourceQualifiers::parse("values");
  EXPECT_FALSE(qualifiers.locale.has_value());
  EXPECT_FALSE(qualifiers.density.has_value());
}

TEST(ResourceQualifiersTest, ParseLocaleQualifiers) {
  auto qualifiers = ResourceQualifiers::parse("values-en");
  EXPECT_TRUE(qualifiers.locale.has_value());
  EXPECT_EQ(*qualifiers.locale, "en");
}

TEST(ResourceQualifiersTest, ParseLocaleAndRegion) {
  auto qualifiers = ResourceQualifiers::parse("values-en-rUS");
  EXPECT_TRUE(qualifiers.locale.has_value());
  EXPECT_EQ(*qualifiers.locale, "en");
  EXPECT_TRUE(qualifiers.region.has_value());
  EXPECT_EQ(*qualifiers.region, "US");
}

TEST(ResourceQualifiersTest, ParseDensityQualifiers) {
  auto qualifiers = ResourceQualifiers::parse("drawable-mdpi");
  EXPECT_TRUE(qualifiers.density.has_value());
  EXPECT_EQ(*qualifiers.density, ResourceDensity::MDPI);
  
  qualifiers = ResourceQualifiers::parse("drawable-xhdpi");
  EXPECT_TRUE(qualifiers.density.has_value());
  EXPECT_EQ(*qualifiers.density, ResourceDensity::XHDPI);
}

TEST(ResourceQualifiersTest, ParseApiLevel) {
  auto qualifiers = ResourceQualifiers::parse("values-v21");
  EXPECT_TRUE(qualifiers.api_level.has_value());
  EXPECT_EQ(*qualifiers.api_level, 21);
}

TEST(ResourceQualifiersTest, ParseComplexQualifiers) {
  auto qualifiers = ResourceQualifiers::parse("values-en-rUS-mdpi-v21");
  EXPECT_TRUE(qualifiers.locale.has_value());
  EXPECT_EQ(*qualifiers.locale, "en");
  EXPECT_TRUE(qualifiers.region.has_value());
  EXPECT_EQ(*qualifiers.region, "US");
  EXPECT_TRUE(qualifiers.density.has_value());
  EXPECT_EQ(*qualifiers.density, ResourceDensity::MDPI);
  EXPECT_TRUE(qualifiers.api_level.has_value());
  EXPECT_EQ(*qualifiers.api_level, 21);
}

TEST(ResourceQualifiersTest, ToString) {
  ResourceQualifiers qualifiers;
  qualifiers.locale = "en";
  qualifiers.region = "US";
  qualifiers.density = ResourceDensity::HDPI;
  qualifiers.api_level = 28;
  
  auto str = qualifiers.to_string();
  EXPECT_NE(str.find("en"), std::string::npos);
  EXPECT_NE(str.find("US"), std::string::npos);
  EXPECT_NE(str.find("hdpi"), std::string::npos);
  EXPECT_NE(str.find("v28"), std::string::npos);
}

// Test ResourceFile type detection
TEST_F(ResourceTestFixture, ResourceFileGetType) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  create_test_resource("res/layout/activity_main.xml", "<LinearLayout/>");
  create_test_resource("res/drawable/icon.png", "PNG");
  
  auto values_type = ResourceFile::get_type(test_dir_ / "res/values/strings.xml");
  EXPECT_EQ(values_type, ResourceType::Values);
  
  auto layout_type = ResourceFile::get_type(test_dir_ / "res/layout/activity_main.xml");
  EXPECT_EQ(layout_type, ResourceType::Layout);
  
  auto drawable_type = ResourceFile::get_type(test_dir_ / "res/drawable/icon.png");
  EXPECT_EQ(drawable_type, ResourceType::Drawable);
}

TEST_F(ResourceTestFixture, ResourceFileGetTypeWithQualifiers) {
  create_test_resource("res/values-en/strings.xml", "<resources></resources>");
  create_test_resource("res/layout-land/activity_main.xml", "<LinearLayout/>");
  create_test_resource("res/drawable-hdpi/icon.png", "PNG");
  
  auto values_type = ResourceFile::get_type(test_dir_ / "res/values-en/strings.xml");
  EXPECT_EQ(values_type, ResourceType::Values);
  
  auto layout_type = ResourceFile::get_type(test_dir_ / "res/layout-land/activity_main.xml");
  EXPECT_EQ(layout_type, ResourceType::Layout);
  
  auto drawable_type = ResourceFile::get_type(test_dir_ / "res/drawable-hdpi/icon.png");
  EXPECT_EQ(drawable_type, ResourceType::Drawable);
}

// Test ResourceFile hash computation
TEST_F(ResourceTestFixture, ResourceFileComputeHash) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  
  auto path = test_dir_ / "res/values/strings.xml";
  auto hash_result = ResourceFile::compute_hash(path);
  
  ASSERT_TRUE(hash_result.has_value());
  EXPECT_FALSE(hash_result->empty());
  EXPECT_EQ(hash_result->length(), 64); // SHA-256 produces 64 hex chars
}

TEST_F(ResourceTestFixture, ResourceFileComputeHashDeterministic) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  
  auto path = test_dir_ / "res/values/strings.xml";
  auto hash1 = ResourceFile::compute_hash(path);
  auto hash2 = ResourceFile::compute_hash(path);
  
  ASSERT_TRUE(hash1.has_value());
  ASSERT_TRUE(hash2.has_value());
  EXPECT_EQ(*hash1, *hash2);
}

TEST_F(ResourceTestFixture, ResourceFileComputeHashDifferentContent) {
  create_test_resource("res/values/strings1.xml", "<resources><string name=\"a\">A</string></resources>");
  create_test_resource("res/values/strings2.xml", "<resources><string name=\"b\">B</string></resources>");
  
  auto hash1 = ResourceFile::compute_hash(test_dir_ / "res/values/strings1.xml");
  auto hash2 = ResourceFile::compute_hash(test_dir_ / "res/values/strings2.xml");
  
  ASSERT_TRUE(hash1.has_value());
  ASSERT_TRUE(hash2.has_value());
  EXPECT_NE(*hash1, *hash2);
}

TEST_F(ResourceTestFixture, ResourceFileComputeHashNonexistent) {
  auto path = test_dir_ / "nonexistent.xml";
  auto hash_result = ResourceFile::compute_hash(path);
  
  EXPECT_FALSE(hash_result.has_value());
  EXPECT_EQ(hash_result.error(), AndroidResourceError::IoError);
}

// Test AndroidManifest parsing
TEST_F(ResourceTestFixture, ManifestParseBasic) {
  auto manifest_path = test_dir_ / "AndroidManifest.xml";
  create_test_manifest(manifest_path);
  
  auto result = AndroidManifest::parse(manifest_path);
  ASSERT_TRUE(result.has_value());
  
  EXPECT_EQ(result->package_name, "com.example.test");
  EXPECT_EQ(result->version_code, 1);
  EXPECT_EQ(result->version_name, "1.0");
  EXPECT_EQ(result->min_sdk_version, 21);
  EXPECT_EQ(result->target_sdk_version, 34);
  EXPECT_FALSE(result->content_hash.empty());
}

TEST_F(ResourceTestFixture, ManifestParseNonexistent) {
  auto manifest_path = test_dir_ / "NonexistentManifest.xml";
  auto result = AndroidManifest::parse(manifest_path);
  
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidResourceError::InvalidManifest);
}

// Test resource scanning
TEST_F(ResourceTestFixture, ScanResourcesEmpty) {
  auto res_dir = test_dir_ / "res";
  std::filesystem::create_directories(res_dir);
  
  auto result = resource_utils::scan_resources(res_dir);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->empty());
}

TEST_F(ResourceTestFixture, ScanResourcesMultipleFiles) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  create_test_resource("res/values/colors.xml", "<resources></resources>");
  create_test_resource("res/layout/activity_main.xml", "<LinearLayout/>");
  
  auto res_dir = test_dir_ / "res";
  auto result = resource_utils::scan_resources(res_dir);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 3);
}

TEST_F(ResourceTestFixture, ScanResourcesWithQualifiers) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  create_test_resource("res/values-en/strings.xml", "<resources></resources>");
  create_test_resource("res/values-es/strings.xml", "<resources></resources>");
  
  auto res_dir = test_dir_ / "res";
  auto result = resource_utils::scan_resources(res_dir);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 3);
  
  // Check that qualifiers are parsed
  for (const auto& resource : *result) {
    if (resource.path.parent_path().filename() == "values-en") {
      EXPECT_TRUE(resource.qualifiers.locale.has_value());
      EXPECT_EQ(*resource.qualifiers.locale, "en");
    }
  }
}

// Test resource sorting
TEST_F(ResourceTestFixture, SortResourcesDeterministic) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  create_test_resource("res/layout/activity_main.xml", "<LinearLayout/>");
  create_test_resource("res/drawable/icon.png", "PNG");
  
  auto res_dir = test_dir_ / "res";
  auto result = resource_utils::scan_resources(res_dir);
  ASSERT_TRUE(result.has_value());
  
  auto resources1 = *result;
  auto resources2 = *result;
  
  resource_utils::sort_resources(resources1);
  resource_utils::sort_resources(resources2);
  
  ASSERT_EQ(resources1.size(), resources2.size());
  for (size_t i = 0; i < resources1.size(); ++i) {
    EXPECT_EQ(resources1[i].path, resources2[i].path);
  }
}

// Test resource filtering
TEST_F(ResourceTestFixture, FilterByType) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  create_test_resource("res/layout/activity_main.xml", "<LinearLayout/>");
  create_test_resource("res/drawable/icon.png", "PNG");
  
  auto res_dir = test_dir_ / "res";
  auto scan_result = resource_utils::scan_resources(res_dir);
  ASSERT_TRUE(scan_result.has_value());
  
  auto values = resource_utils::filter_by_type(*scan_result, ResourceType::Values);
  EXPECT_EQ(values.size(), 1);
  EXPECT_EQ(values[0].type, ResourceType::Values);
  
  auto layouts = resource_utils::filter_by_type(*scan_result, ResourceType::Layout);
  EXPECT_EQ(layouts.size(), 1);
  EXPECT_EQ(layouts[0].type, ResourceType::Layout);
}

// Test config validation
TEST(AndroidResourceProcessorTest, ValidateCompileConfigEmpty) {
  Aapt2CompileConfig config;
  config.output_dir = "/tmp/output";
  
  auto result = AndroidResourceProcessor::validate_compile_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidResourceError::InvalidConfiguration);
}

TEST(AndroidResourceProcessorTest, ValidateCompileConfigNonexistentFile) {
  Aapt2CompileConfig config;
  
  ResourceFile resource;
  resource.path = "/nonexistent/file.xml";
  config.resources.push_back(resource);
  config.output_dir = "/tmp/output";
  
  auto result = AndroidResourceProcessor::validate_compile_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidResourceError::InvalidResourceFile);
}

TEST(AndroidResourceProcessorTest, ValidateLinkConfigEmpty) {
  Aapt2LinkConfig config;
  config.manifest = "/tmp/manifest.xml";
  config.android_jar = "/tmp/android.jar";
  config.output_apk = "/tmp/output.apk";
  
  auto result = AndroidResourceProcessor::validate_link_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AndroidResourceError::InvalidConfiguration);
}

// Test hash computation
TEST_F(ResourceTestFixture, ComputeCompileHashDeterministic) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  
  Aapt2CompileConfig config;
  ResourceFile resource;
  resource.path = test_dir_ / "res/values/strings.xml";
  resource.content_hash = "abc123";
  config.resources.push_back(resource);
  config.output_dir = test_dir_ / "output";
  
  auto hash1 = AndroidResourceProcessor::compute_compile_hash(config);
  auto hash2 = AndroidResourceProcessor::compute_compile_hash(config);
  
  EXPECT_EQ(hash1, hash2);
  EXPECT_FALSE(hash1.empty());
  EXPECT_EQ(hash1.length(), 64); // SHA-256
}

TEST_F(ResourceTestFixture, ComputeCompileHashChangesWithContent) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  
  Aapt2CompileConfig config1;
  ResourceFile resource1;
  resource1.path = test_dir_ / "res/values/strings.xml";
  resource1.content_hash = "abc123";
  config1.resources.push_back(resource1);
  config1.output_dir = test_dir_ / "output";
  
  Aapt2CompileConfig config2;
  ResourceFile resource2;
  resource2.path = test_dir_ / "res/values/strings.xml";
  resource2.content_hash = "def456";
  config2.resources.push_back(resource2);
  config2.output_dir = test_dir_ / "output";
  
  auto hash1 = AndroidResourceProcessor::compute_compile_hash(config1);
  auto hash2 = AndroidResourceProcessor::compute_compile_hash(config2);
  
  EXPECT_NE(hash1, hash2);
}

// Test incremental compilation support
TEST_F(ResourceTestFixture, IncrementalComputeResourceHash) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  
  auto path = test_dir_ / "res/values/strings.xml";
  auto hash = resource_incremental::compute_resource_hash(path);
  
  EXPECT_FALSE(hash.empty());
  EXPECT_EQ(hash.length(), 64);
}

TEST_F(ResourceTestFixture, IncrementalHasResourceChanged) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  
  auto path = test_dir_ / "res/values/strings.xml";
  auto hash1 = resource_incremental::compute_resource_hash(path);
  
  // File hasn't changed
  EXPECT_FALSE(resource_incremental::has_resource_changed(path, hash1));
  
  // File has changed
  EXPECT_TRUE(resource_incremental::has_resource_changed(path, "different_hash"));
}

TEST_F(ResourceTestFixture, IncrementalSaveAndLoadState) {
  create_test_resource("res/values/strings.xml", "<resources></resources>");
  
  Aapt2CompileConfig config;
  ResourceFile resource;
  resource.path = test_dir_ / "res/values/strings.xml";
  resource.content_hash = "abc123";
  config.resources.push_back(resource);
  config.output_dir = test_dir_ / "output";
  
  auto state_file = test_dir_ / ".horcrux_resource_state";
  auto compilation_hash = "test_hash_12345";
  
  // Save state
  bool saved = resource_incremental::save_compilation_state(
    state_file, config, compilation_hash);
  EXPECT_TRUE(saved);
  
  // Load state
  auto loaded_state = resource_incremental::load_compilation_state(state_file);
  ASSERT_TRUE(loaded_state.has_value());
  EXPECT_EQ(loaded_state->compilation_hash, compilation_hash);
  EXPECT_EQ(loaded_state->resource_hashes.size(), 1);
}

TEST_F(ResourceTestFixture, IncrementalLoadStateNonexistent) {
  auto state_file = test_dir_ / "nonexistent_state";
  auto loaded_state = resource_incremental::load_compilation_state(state_file);
  
  EXPECT_FALSE(loaded_state.has_value());
}
