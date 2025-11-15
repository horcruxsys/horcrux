// Horcrux - Android Variant Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <gtest/gtest.h>

#include "../src/core/android_variant.h"

namespace horcrux::core::test {

class AndroidVariantTestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    // Set up base configuration
    base_config_.base_application_id = "com.example.app";
    base_config_.base_version_code = 1;
    base_config_.base_version_name = "1.0";
    base_config_.base_min_sdk = 21;
    base_config_.base_target_sdk = 34;

    // Add base source directories
    base_config_.base_source_dirs.emplace_back("src/main/java");
    base_config_.base_source_dirs.emplace_back("src/main/kotlin");

    // Add base resource directories
    base_config_.base_resource_dirs.emplace_back("src/main/res");

    // Add base manifest
    base_config_.base_manifest_files.emplace_back("src/main/AndroidManifest.xml");
  }

  VariantMatrixConfig base_config_;
};

// Test BuildType functionality
TEST_F(AndroidVariantTestFixture, BuildTypeBasicProperties) {
  BuildType debug;
  debug.name = "debug";
  debug.debuggable = true;
  debug.minify_enabled = false;

  EXPECT_EQ(debug.name, "debug");
  EXPECT_TRUE(debug.debuggable);
  EXPECT_FALSE(debug.minify_enabled);
}

// Test ProductFlavor functionality
TEST_F(AndroidVariantTestFixture, ProductFlavorBasicProperties) {
  ProductFlavor free;
  free.name = "free";
  free.dimension = "tier";
  free.application_id_suffix = ".free";

  EXPECT_EQ(free.name, "free");
  EXPECT_EQ(free.dimension, "tier");
  EXPECT_EQ(*free.application_id_suffix, ".free");
}

// Test variant name generation
TEST_F(AndroidVariantTestFixture, VariantNameGenerationNoFlavors) {
  BuildType debug;
  debug.name = "debug";

  BuildType release;
  release.name = "release";

  base_config_.build_types.push_back(debug);
  base_config_.build_types.push_back(release);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  EXPECT_EQ(matrix.variants.size(), 2);

  auto names = matrix.get_variant_names();
  EXPECT_TRUE(std::find(names.begin(), names.end(), "debug") != names.end());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "release") != names.end());
}

// Test variant name generation with one dimension
TEST_F(AndroidVariantTestFixture, VariantNameGenerationOneDimension) {
  BuildType debug;
  debug.name = "debug";

  BuildType release;
  release.name = "release";

  ProductFlavor free;
  free.name = "free";
  free.dimension = "tier";

  ProductFlavor pro;
  pro.name = "pro";
  pro.dimension = "tier";

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.build_types.push_back(debug);
  base_config_.build_types.push_back(release);
  base_config_.flavors.push_back(free);
  base_config_.flavors.push_back(pro);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  EXPECT_EQ(matrix.variants.size(), 4); // 2 flavors × 2 build types = 4 variants

  auto names = matrix.get_variant_names();
  EXPECT_TRUE(std::find(names.begin(), names.end(), "freeDebug") != names.end());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "freeRelease") != names.end());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "proDebug") != names.end());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "proRelease") != names.end());
}

// Test variant name generation with multiple dimensions
TEST_F(AndroidVariantTestFixture, VariantNameGenerationMultipleDimensions) {
  BuildType debug;
  debug.name = "debug";

  ProductFlavor free;
  free.name = "free";
  free.dimension = "tier";

  ProductFlavor pro;
  pro.name = "pro";
  pro.dimension = "tier";

  ProductFlavor google;
  google.name = "google";
  google.dimension = "store";

  ProductFlavor amazon;
  amazon.name = "amazon";
  amazon.dimension = "store";

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.flavor_dimensions.push_back("store");
  base_config_.build_types.push_back(debug);
  base_config_.flavors.push_back(free);
  base_config_.flavors.push_back(pro);
  base_config_.flavors.push_back(google);
  base_config_.flavors.push_back(amazon);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  EXPECT_EQ(matrix.variants.size(), 4); // 2 tier × 2 store × 1 build type = 4 variants

  auto names = matrix.get_variant_names();
  EXPECT_TRUE(std::find(names.begin(), names.end(), "freeGoogleDebug") != names.end());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "freeAmazonDebug") != names.end());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "proGoogleDebug") != names.end());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "proAmazonDebug") != names.end());
}

// Test application ID merging
TEST_F(AndroidVariantTestFixture, ApplicationIdMerging) {
  BuildType debug;
  debug.name = "debug";
  debug.application_id_suffix = ".debug";

  ProductFlavor free;
  free.name = "free";
  free.dimension = "tier";
  free.application_id_suffix = ".free";

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.build_types.push_back(debug);
  base_config_.flavors.push_back(free);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  ASSERT_EQ(matrix.variants.size(), 1);

  const auto& variant = matrix.variants[0];
  EXPECT_EQ(variant.application_id, "com.example.app.free.debug");
}

// Test version name merging
TEST_F(AndroidVariantTestFixture, VersionNameMerging) {
  BuildType debug;
  debug.name = "debug";
  debug.version_name_suffix = "-debug";

  base_config_.build_types.push_back(debug);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  ASSERT_EQ(matrix.variants.size(), 1);

  const auto& variant = matrix.variants[0];
  EXPECT_EQ(variant.version_name, "1.0-debug");
}

// Test flavor overrides application ID
TEST_F(AndroidVariantTestFixture, FlavorOverridesApplicationId) {
  BuildType debug;
  debug.name = "debug";

  ProductFlavor pro;
  pro.name = "pro";
  pro.dimension = "tier";
  pro.application_id = "com.example.pro";

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.build_types.push_back(debug);
  base_config_.flavors.push_back(pro);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  ASSERT_EQ(matrix.variants.size(), 1);

  const auto& variant = matrix.variants[0];
  EXPECT_EQ(variant.application_id, "com.example.pro");
}

// Test flavor overrides version code
TEST_F(AndroidVariantTestFixture, FlavorOverridesVersionCode) {
  BuildType debug;
  debug.name = "debug";

  ProductFlavor pro;
  pro.name = "pro";
  pro.dimension = "tier";
  pro.version_code = 100;

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.build_types.push_back(debug);
  base_config_.flavors.push_back(pro);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  ASSERT_EQ(matrix.variants.size(), 1);

  const auto& variant = matrix.variants[0];
  EXPECT_EQ(variant.version_code, 100);
}

// Test source directory merging
TEST_F(AndroidVariantTestFixture, SourceDirectoryMerging) {
  BuildType debug;
  debug.name = "debug";
  debug.source_dirs.emplace_back("src/debug/java");

  ProductFlavor free;
  free.name = "free";
  free.dimension = "tier";
  free.source_dirs.emplace_back("src/free/java");

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.build_types.push_back(debug);
  base_config_.flavors.push_back(free);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  ASSERT_EQ(matrix.variants.size(), 1);

  const auto& variant = matrix.variants[0];
  EXPECT_EQ(variant.source_dirs.size(), 4); // base (2) + flavor (1) + build type (1)

  // Check all source dirs are present
  std::vector<std::string> expected_dirs = {"src/main/java", "src/main/kotlin", "src/free/java",
                                            "src/debug/java"};
  for (const auto& expected : expected_dirs) {
    bool found = false;
    for (const auto& dir : variant.source_dirs) {
      if (dir.string() == expected) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << "Expected source dir not found: " << expected;
  }
}

// Test resource directory merging
TEST_F(AndroidVariantTestFixture, ResourceDirectoryMerging) {
  BuildType debug;
  debug.name = "debug";
  debug.resource_dirs.emplace_back("src/debug/res");

  ProductFlavor free;
  free.name = "free";
  free.dimension = "tier";
  free.resource_dirs.emplace_back("src/free/res");

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.build_types.push_back(debug);
  base_config_.flavors.push_back(free);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  ASSERT_EQ(matrix.variants.size(), 1);

  const auto& variant = matrix.variants[0];
  EXPECT_EQ(variant.resource_dirs.size(), 3); // base (1) + flavor (1) + build type (1)

  // Check all resource dirs are present
  std::vector<std::string> expected_dirs = {"src/main/res", "src/free/res", "src/debug/res"};
  for (const auto& expected : expected_dirs) {
    bool found = false;
    for (const auto& dir : variant.resource_dirs) {
      if (dir.string() == expected) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << "Expected resource dir not found: " << expected;
  }
}

// Test manifest file merging
TEST_F(AndroidVariantTestFixture, ManifestFileMerging) {
  BuildType debug;
  debug.name = "debug";
  debug.manifest_files.emplace_back("src/debug/AndroidManifest.xml");

  ProductFlavor free;
  free.name = "free";
  free.dimension = "tier";
  free.manifest_files.emplace_back("src/free/AndroidManifest.xml");

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.build_types.push_back(debug);
  base_config_.flavors.push_back(free);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  ASSERT_EQ(matrix.variants.size(), 1);

  const auto& variant = matrix.variants[0];
  EXPECT_EQ(variant.manifest_files.size(), 3); // base (1) + flavor (1) + build type (1)
}

// Test SDK version merging
TEST_F(AndroidVariantTestFixture, SdkVersionMerging) {
  BuildType debug;
  debug.name = "debug";

  ProductFlavor free;
  free.name = "free";
  free.dimension = "tier";
  free.min_sdk = 24;
  free.target_sdk = 34;

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.build_types.push_back(debug);
  base_config_.flavors.push_back(free);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  ASSERT_EQ(matrix.variants.size(), 1);

  const auto& variant = matrix.variants[0];
  EXPECT_EQ(*variant.min_sdk, 24);
  EXPECT_EQ(*variant.target_sdk, 34);
}

// Test build flags
TEST_F(AndroidVariantTestFixture, BuildFlags) {
  BuildType debug;
  debug.name = "debug";
  debug.debuggable = true;
  debug.minify_enabled = false;
  debug.shrink_resources = false;

  BuildType release;
  release.name = "release";
  release.debuggable = false;
  release.minify_enabled = true;
  release.shrink_resources = true;

  base_config_.build_types.push_back(debug);
  base_config_.build_types.push_back(release);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  EXPECT_EQ(matrix.variants.size(), 2);

  // Find debug variant
  auto debug_variant = matrix.find_variant("debug");
  ASSERT_TRUE(debug_variant.has_value());
  EXPECT_TRUE(debug_variant->debuggable);
  EXPECT_FALSE(debug_variant->minify_enabled);
  EXPECT_FALSE(debug_variant->shrink_resources);

  // Find release variant
  auto release_variant = matrix.find_variant("release");
  ASSERT_TRUE(release_variant.has_value());
  EXPECT_FALSE(release_variant->debuggable);
  EXPECT_TRUE(release_variant->minify_enabled);
  EXPECT_TRUE(release_variant->shrink_resources);
}

// Test variant hash computation is deterministic
TEST_F(AndroidVariantTestFixture, VariantHashIsDeterministic) {
  BuildType debug;
  debug.name = "debug";

  base_config_.build_types.push_back(debug);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  ASSERT_EQ(matrix.variants.size(), 1);

  const auto& variant = matrix.variants[0];
  auto hash1 = variant.compute_hash();
  auto hash2 = variant.compute_hash();

  EXPECT_EQ(hash1, hash2);
  EXPECT_FALSE(hash1.empty());
  EXPECT_EQ(hash1.length(), 64); // SHA-256 produces 64 hex characters
}

// Test validation: no build types
TEST_F(AndroidVariantTestFixture, ValidationNoBuildTypes) {
  auto validation = base_config_.validate();
  EXPECT_FALSE(validation.has_value());
  EXPECT_TRUE(validation.error().find("build type") != std::string::npos);
}

// Test validation: invalid flavor dimension
TEST_F(AndroidVariantTestFixture, ValidationInvalidFlavorDimension) {
  BuildType debug;
  debug.name = "debug";

  ProductFlavor free;
  free.name = "free";
  free.dimension = "invalid_dimension";

  base_config_.build_types.push_back(debug);
  base_config_.flavor_dimensions.push_back("tier");
  base_config_.flavors.push_back(free);

  auto validation = base_config_.validate();
  EXPECT_FALSE(validation.has_value());
  EXPECT_TRUE(validation.error().find("invalid dimension") != std::string::npos);
}

// Test validation: dimension with no flavors
TEST_F(AndroidVariantTestFixture, ValidationDimensionWithNoFlavors) {
  BuildType debug;
  debug.name = "debug";

  base_config_.build_types.push_back(debug);
  base_config_.flavor_dimensions.push_back("tier");

  auto validation = base_config_.validate();
  EXPECT_FALSE(validation.has_value());
  EXPECT_TRUE(validation.error().find("has no flavors") != std::string::npos);
}

// Test finding variant by name
TEST_F(AndroidVariantTestFixture, FindVariantByName) {
  BuildType debug;
  debug.name = "debug";

  BuildType release;
  release.name = "release";

  base_config_.build_types.push_back(debug);
  base_config_.build_types.push_back(release);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;

  auto debug_variant = matrix.find_variant("debug");
  ASSERT_TRUE(debug_variant.has_value());
  EXPECT_EQ(debug_variant->name, "debug");

  auto release_variant = matrix.find_variant("release");
  ASSERT_TRUE(release_variant.has_value());
  EXPECT_EQ(release_variant->name, "release");

  auto nonexistent = matrix.find_variant("nonexistent");
  EXPECT_FALSE(nonexistent.has_value());
}

// Test complex multi-dimensional scenario
TEST_F(AndroidVariantTestFixture, ComplexMultiDimensionalScenario) {
  // Set up build types
  BuildType debug;
  debug.name = "debug";
  debug.debuggable = true;
  debug.application_id_suffix = ".debug";

  BuildType release;
  release.name = "release";
  release.minify_enabled = true;

  // Set up tier flavors
  ProductFlavor free;
  free.name = "free";
  free.dimension = "tier";
  free.application_id_suffix = ".free";

  ProductFlavor pro;
  pro.name = "pro";
  pro.dimension = "tier";
  pro.application_id_suffix = ".pro";
  pro.version_code = 200;

  // Set up store flavors
  ProductFlavor google;
  google.name = "google";
  google.dimension = "store";

  ProductFlavor amazon;
  amazon.name = "amazon";
  amazon.dimension = "store";
  amazon.application_id_suffix = ".amazon";

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.flavor_dimensions.push_back("store");
  base_config_.build_types.push_back(debug);
  base_config_.build_types.push_back(release);
  base_config_.flavors.push_back(free);
  base_config_.flavors.push_back(pro);
  base_config_.flavors.push_back(google);
  base_config_.flavors.push_back(amazon);

  auto result = AndroidVariantBuilder::build_matrix(base_config_);
  ASSERT_TRUE(result.has_value());

  auto& matrix = *result;
  EXPECT_EQ(matrix.variants.size(), 8); // 2 tier × 2 store × 2 build types = 8 variants

  // Check a specific variant: proAmazonDebug
  auto variant = matrix.find_variant("proAmazonDebug");
  ASSERT_TRUE(variant.has_value());
  EXPECT_EQ(variant->name, "proAmazonDebug");
  EXPECT_EQ(variant->application_id, "com.example.app.pro.amazon.debug");
  EXPECT_EQ(variant->version_code, 200);
  EXPECT_TRUE(variant->debuggable);
  EXPECT_FALSE(variant->minify_enabled);

  // Check another variant: freeGoogleRelease
  variant = matrix.find_variant("freeGoogleRelease");
  ASSERT_TRUE(variant.has_value());
  EXPECT_EQ(variant->name, "freeGoogleRelease");
  EXPECT_EQ(variant->application_id, "com.example.app.free");
  EXPECT_EQ(variant->version_code, 1);
  EXPECT_FALSE(variant->debuggable);
  EXPECT_TRUE(variant->minify_enabled);
}

// Test deterministic variant ordering
TEST_F(AndroidVariantTestFixture, DeterministicVariantOrdering) {
  BuildType debug;
  debug.name = "debug";

  ProductFlavor free;
  free.name = "free";
  free.dimension = "tier";

  ProductFlavor pro;
  pro.name = "pro";
  pro.dimension = "tier";

  base_config_.flavor_dimensions.push_back("tier");
  base_config_.build_types.push_back(debug);
  base_config_.flavors.push_back(free);
  base_config_.flavors.push_back(pro);

  // Build matrix twice
  auto result1 = AndroidVariantBuilder::build_matrix(base_config_);
  auto result2 = AndroidVariantBuilder::build_matrix(base_config_);

  ASSERT_TRUE(result1.has_value());
  ASSERT_TRUE(result2.has_value());

  auto names1 = result1->get_variant_names();
  auto names2 = result2->get_variant_names();

  EXPECT_EQ(names1, names2);
}

} // namespace horcrux::core::test
