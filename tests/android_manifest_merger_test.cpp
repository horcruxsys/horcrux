// Horcrux - Android Manifest Merger Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "android_manifest_merger.h"
#include "android_resources.h"

using namespace horcrux::core;

// Helper function to create test manifests
class ManifestMergerTestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    // Create temporary test directory
    test_dir_ = std::filesystem::temp_directory_path() / "horcrux_manifest_merger_test";
    std::filesystem::create_directories(test_dir_);
  }

  void TearDown() override {
    // Clean up test directory
    if (std::filesystem::exists(test_dir_)) {
      std::filesystem::remove_all(test_dir_);
    }
  }

  std::filesystem::path test_dir_;

  // Helper to create a test manifest file
  void create_manifest(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    file << content;
    file.close();
  }

  // Helper to read manifest content
  std::string read_manifest(const std::filesystem::path& path) {
    std::ifstream file(path);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  }

  // Helper to create a minimal valid manifest
  std::string minimal_manifest(const std::string& package_name = "com.example.test") {
    return R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package=")" +
           package_name + R"(">
    <application />
</manifest>)";
  }

  // Helper to create a manifest with activity
  std::string manifest_with_activity(const std::string& activity_name) {
    return R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.test">
    <application>
        <activity android:name=")" +
           activity_name + R"(" />
    </application>
</manifest>)";
  }

  // Helper to create a manifest with permissions
  std::string manifest_with_permissions(const std::vector<std::string>& permissions) {
    std::string result = R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.test">
)";
    for (const auto& perm : permissions) {
      result += "    <uses-permission android:name=\"" + perm + "\" />\n";
    }
    result += "    <application />\n</manifest>";
    return result;
  }
};

// Test ParsedManifest parsing
TEST_F(ManifestMergerTestFixture, ParseValidManifest) {
  auto manifest_path = test_dir_ / "AndroidManifest.xml";
  create_manifest(manifest_path, minimal_manifest());

  auto result = ParsedManifest::parse(manifest_path, ManifestPriority::Main);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->source_path, manifest_path);
  EXPECT_EQ(result->priority, ManifestPriority::Main);
  EXPECT_NE(result->root, nullptr);
  EXPECT_EQ(std::string(result->root->Name()), "manifest");
}

TEST_F(ManifestMergerTestFixture, ParseNonexistentManifest) {
  auto manifest_path = test_dir_ / "NonexistentManifest.xml";
  auto result = ParsedManifest::parse(manifest_path, ManifestPriority::Main);
  EXPECT_FALSE(result.has_value());
}

TEST_F(ManifestMergerTestFixture, ParseInvalidXml) {
  auto manifest_path = test_dir_ / "InvalidManifest.xml";
  create_manifest(manifest_path, "This is not valid XML");

  auto result = ParsedManifest::parse(manifest_path, ManifestPriority::Main);
  EXPECT_FALSE(result.has_value());
}

// Test basic manifest merging
TEST_F(ManifestMergerTestFixture, MergeMainManifestOnly) {
  auto main_manifest = test_dir_ / "main/AndroidManifest.xml";
  auto output_manifest = test_dir_ / "merged/AndroidManifest.xml";

  create_manifest(main_manifest, minimal_manifest());

  AndroidManifestMerger merger;
  AndroidManifestMerger::MergeConfig config;
  config.main_manifest = main_manifest;
  config.output_manifest = output_manifest;

  auto result = merger.merge(config);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->success);
  EXPECT_TRUE(std::filesystem::exists(output_manifest));
  EXPECT_EQ(result->errors.size(), 0);
}

TEST_F(ManifestMergerTestFixture, MergeWithLibraryManifest) {
  auto main_manifest = test_dir_ / "main/AndroidManifest.xml";
  auto lib_manifest = test_dir_ / "lib/AndroidManifest.xml";
  auto output_manifest = test_dir_ / "merged/AndroidManifest.xml";

  create_manifest(main_manifest, manifest_with_activity("MainActivity"));
  create_manifest(lib_manifest, manifest_with_activity("LibActivity"));

  AndroidManifestMerger merger;
  AndroidManifestMerger::MergeConfig config;
  config.main_manifest = main_manifest;
  config.library_manifests.push_back(lib_manifest);
  config.output_manifest = output_manifest;

  auto result = merger.merge(config);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->success);

  // Verify merged manifest contains both activities
  std::string merged_content = read_manifest(output_manifest);
  EXPECT_NE(merged_content.find("MainActivity"), std::string::npos);
  EXPECT_NE(merged_content.find("LibActivity"), std::string::npos);
}

TEST_F(ManifestMergerTestFixture, MergePermissions) {
  auto main_manifest = test_dir_ / "main/AndroidManifest.xml";
  auto lib_manifest = test_dir_ / "lib/AndroidManifest.xml";
  auto output_manifest = test_dir_ / "merged/AndroidManifest.xml";

  create_manifest(main_manifest, manifest_with_permissions({"android.permission.INTERNET"}));
  create_manifest(lib_manifest, manifest_with_permissions({"android.permission.CAMERA"}));

  AndroidManifestMerger merger;
  AndroidManifestMerger::MergeConfig config;
  config.main_manifest = main_manifest;
  config.library_manifests.push_back(lib_manifest);
  config.output_manifest = output_manifest;

  auto result = merger.merge(config);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->success);

  // Verify merged manifest contains both permissions
  std::string merged_content = read_manifest(output_manifest);
  EXPECT_NE(merged_content.find("android.permission.INTERNET"), std::string::npos);
  EXPECT_NE(merged_content.find("android.permission.CAMERA"), std::string::npos);
}

TEST_F(ManifestMergerTestFixture, MergeDuplicatePermissionsDeduplicates) {
  auto main_manifest = test_dir_ / "main/AndroidManifest.xml";
  auto lib_manifest = test_dir_ / "lib/AndroidManifest.xml";
  auto output_manifest = test_dir_ / "merged/AndroidManifest.xml";

  create_manifest(main_manifest, manifest_with_permissions({"android.permission.INTERNET"}));
  create_manifest(lib_manifest, manifest_with_permissions({"android.permission.INTERNET"}));

  AndroidManifestMerger merger;
  AndroidManifestMerger::MergeConfig config;
  config.main_manifest = main_manifest;
  config.library_manifests.push_back(lib_manifest);
  config.output_manifest = output_manifest;

  auto result = merger.merge(config);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->success);

  // Count occurrences of the permission (should be 1 after merge)
  std::string merged_content = read_manifest(output_manifest);
  size_t count = 0;
  size_t pos = 0;
  std::string search = "android.permission.INTERNET";
  while ((pos = merged_content.find(search, pos)) != std::string::npos) {
    count++;
    pos += search.length();
  }
  EXPECT_EQ(count, 1);
}

TEST_F(ManifestMergerTestFixture, MergeMultipleLibraries) {
  auto main_manifest = test_dir_ / "main/AndroidManifest.xml";
  auto lib1_manifest = test_dir_ / "lib1/AndroidManifest.xml";
  auto lib2_manifest = test_dir_ / "lib2/AndroidManifest.xml";
  auto output_manifest = test_dir_ / "merged/AndroidManifest.xml";

  create_manifest(main_manifest, manifest_with_activity("MainActivity"));
  create_manifest(lib1_manifest, manifest_with_activity("Lib1Activity"));
  create_manifest(lib2_manifest, manifest_with_activity("Lib2Activity"));

  AndroidManifestMerger merger;
  AndroidManifestMerger::MergeConfig config;
  config.main_manifest = main_manifest;
  config.library_manifests.push_back(lib1_manifest);
  config.library_manifests.push_back(lib2_manifest);
  config.output_manifest = output_manifest;

  auto result = merger.merge(config);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->success);

  // Verify merged manifest contains all activities
  std::string merged_content = read_manifest(output_manifest);
  EXPECT_NE(merged_content.find("MainActivity"), std::string::npos);
  EXPECT_NE(merged_content.find("Lib1Activity"), std::string::npos);
  EXPECT_NE(merged_content.find("Lib2Activity"), std::string::npos);
}

TEST_F(ManifestMergerTestFixture, MergePriorityMainOverLibrary) {
  auto main_manifest = test_dir_ / "main/AndroidManifest.xml";
  auto lib_manifest = test_dir_ / "lib/AndroidManifest.xml";
  auto output_manifest = test_dir_ / "merged/AndroidManifest.xml";

  // Both manifests define same activity with different attributes
  std::string main_content = R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.test">
    <application>
        <activity android:name="MainActivity" android:exported="true" />
    </application>
</manifest>)";

  std::string lib_content = R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.lib">
    <application>
        <activity android:name="MainActivity" android:exported="false" />
    </application>
</manifest>)";

  create_manifest(main_manifest, main_content);
  create_manifest(lib_manifest, lib_content);

  AndroidManifestMerger merger;
  AndroidManifestMerger::MergeConfig config;
  config.main_manifest = main_manifest;
  config.library_manifests.push_back(lib_manifest);
  config.output_manifest = output_manifest;

  auto result = merger.merge(config);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->success);

  // Main manifest should win - exported should be "true"
  std::string merged_content = read_manifest(output_manifest);
  EXPECT_NE(merged_content.find("android:exported=\"true\""), std::string::npos);
}

// Test manifest utils
TEST(ManifestUtilsTest, MergeActionConversion) {
  EXPECT_EQ(manifest_utils::merge_action_to_string(MergeAction::Merge), "merge");
  EXPECT_EQ(manifest_utils::merge_action_to_string(MergeAction::Replace), "replace");
  EXPECT_EQ(manifest_utils::merge_action_to_string(MergeAction::Remove), "remove");

  EXPECT_EQ(manifest_utils::string_to_merge_action("merge"), MergeAction::Merge);
  EXPECT_EQ(manifest_utils::string_to_merge_action("replace"), MergeAction::Replace);
  EXPECT_EQ(manifest_utils::string_to_merge_action("remove"), MergeAction::Remove);
}

TEST_F(ManifestMergerTestFixture, DeterministicOrdering) {
  auto main_manifest = test_dir_ / "main/AndroidManifest.xml";
  auto output1 = test_dir_ / "merged1/AndroidManifest.xml";
  auto output2 = test_dir_ / "merged2/AndroidManifest.xml";

  // Create manifest with multiple activities in different order
  std::string manifest_content = R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.test">
    <application>
        <activity android:name="ZActivity" />
        <activity android:name="AActivity" />
        <activity android:name="MActivity" />
    </application>
</manifest>)";

  create_manifest(main_manifest, manifest_content);

  AndroidManifestMerger merger;

  // Merge twice
  AndroidManifestMerger::MergeConfig config1;
  config1.main_manifest = main_manifest;
  config1.output_manifest = output1;
  auto result1 = merger.merge(config1);
  ASSERT_TRUE(result1.has_value());

  AndroidManifestMerger::MergeConfig config2;
  config2.main_manifest = main_manifest;
  config2.output_manifest = output2;
  auto result2 = merger.merge(config2);
  ASSERT_TRUE(result2.has_value());

  // Both outputs should be identical
  std::string content1 = read_manifest(output1);
  std::string content2 = read_manifest(output2);
  EXPECT_EQ(content1, content2);
}

TEST_F(ManifestMergerTestFixture, IntegrationWithResourceProcessor) {
  auto main_manifest = test_dir_ / "main/AndroidManifest.xml";
  auto lib_manifest = test_dir_ / "lib/AndroidManifest.xml";
  auto output_manifest = test_dir_ / "merged/AndroidManifest.xml";

  create_manifest(main_manifest, manifest_with_activity("MainActivity"));
  create_manifest(lib_manifest, manifest_with_activity("LibActivity"));

  // Create a mock toolchain for resource processor
  AndroidToolchain toolchain;
  toolchain.sdk_root = "/mock/sdk";

  AndroidResourceProcessor processor(toolchain);

  ManifestMergeConfig config;
  config.main_manifest = main_manifest;
  config.library_manifests.push_back(lib_manifest);
  config.output_manifest = output_manifest;
  config.verbose = false;

  auto result = processor.merge_manifests(config);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(std::filesystem::exists(output_manifest));

  // Verify merge was successful
  std::string merged_content = read_manifest(output_manifest);
  EXPECT_NE(merged_content.find("MainActivity"), std::string::npos);
  EXPECT_NE(merged_content.find("LibActivity"), std::string::npos);
}
