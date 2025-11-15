// Horcrux - Android Build Variant Support
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

namespace horcrux::core {

// Build type configuration (debug, release, etc.)
struct BuildType {
  std::string name;
  bool debuggable = false;
  bool minify_enabled = false;
  bool shrink_resources = false;
  std::optional<std::string> application_id_suffix;
  std::optional<std::string> version_name_suffix;
  std::map<std::string, std::string> manifest_placeholders;
  std::vector<std::filesystem::path> source_dirs;
  std::vector<std::filesystem::path> resource_dirs;
  std::vector<std::filesystem::path> manifest_files;
};

// Product flavor configuration
struct ProductFlavor {
  std::string name;
  std::string dimension; // Flavor dimension this flavor belongs to
  std::optional<std::string> application_id;
  std::optional<std::string> application_id_suffix;
  std::optional<int> version_code;
  std::optional<std::string> version_name;
  std::optional<int> min_sdk;
  std::optional<int> target_sdk;
  std::map<std::string, std::string> manifest_placeholders;
  std::vector<std::filesystem::path> source_dirs;
  std::vector<std::filesystem::path> resource_dirs;
  std::vector<std::filesystem::path> manifest_files;
};

// Build variant (combination of build type and flavors)
struct BuildVariant {
  std::string name; // e.g., "freeDebug", "proRelease"
  BuildType build_type;
  std::vector<ProductFlavor> flavors; // One flavor per dimension

  // Merged configuration
  std::string application_id;
  int version_code;
  std::string version_name;
  std::optional<int> min_sdk;
  std::optional<int> target_sdk;
  bool debuggable;
  bool minify_enabled;
  bool shrink_resources;

  // Merged source/resource paths
  std::vector<std::filesystem::path> source_dirs;
  std::vector<std::filesystem::path> resource_dirs;
  std::vector<std::filesystem::path> manifest_files;

  // Dependencies specific to this variant
  std::vector<std::string> dependencies;

  // Compute deterministic hash for this variant
  auto compute_hash() const -> std::string;
};

// Variant matrix configuration
struct VariantMatrixConfig {
  std::vector<std::string> flavor_dimensions; // e.g., ["tier", "api"]
  std::vector<BuildType> build_types;
  std::vector<ProductFlavor> flavors; // All flavors across all dimensions

  // Base configuration
  std::string base_application_id;
  int base_version_code = 1;
  std::string base_version_name = "1.0";
  std::optional<int> base_min_sdk;
  std::optional<int> base_target_sdk;

  // Base source/resource paths
  std::vector<std::filesystem::path> base_source_dirs;
  std::vector<std::filesystem::path> base_resource_dirs;
  std::vector<std::filesystem::path> base_manifest_files;

  // Validate configuration
  auto validate() const -> tl::expected<void, std::string>;
};

// Variant matrix expansion result
struct VariantMatrix {
  std::vector<BuildVariant> variants;

  // Find variant by name
  auto find_variant(const std::string& name) const -> std::optional<BuildVariant>;

  // Get all variant names
  auto get_variant_names() const -> std::vector<std::string>;
};

// Android variant matrix builder
class AndroidVariantBuilder {
public:
  // Build variant matrix from configuration
  static auto
  build_matrix(const VariantMatrixConfig& config) -> tl::expected<VariantMatrix, std::string>;

private:
  // Generate all variant combinations
  static auto generate_variants(const VariantMatrixConfig& config) -> std::vector<BuildVariant>;

  // Merge configuration for a specific variant
  static auto merge_variant_config(const VariantMatrixConfig& config, const BuildType& build_type,
                                   const std::vector<ProductFlavor>& flavors) -> BuildVariant;

  // Generate variant name from build type and flavors
  static auto generate_variant_name(const BuildType& build_type,
                                    const std::vector<ProductFlavor>& flavors) -> std::string;

  // Merge application ID with suffixes
  static auto merge_application_id(const std::string& base_id, const BuildType& build_type,
                                   const std::vector<ProductFlavor>& flavors) -> std::string;

  // Merge version name with suffixes
  static auto merge_version_name(const std::string& base_version, const BuildType& build_type,
                                 const std::vector<ProductFlavor>& flavors) -> std::string;

  // Merge source directories
  static auto merge_source_dirs(
      const std::vector<std::filesystem::path>& base_dirs, const BuildType& build_type,
      const std::vector<ProductFlavor>& flavors) -> std::vector<std::filesystem::path>;

  // Merge resource directories
  static auto merge_resource_dirs(
      const std::vector<std::filesystem::path>& base_dirs, const BuildType& build_type,
      const std::vector<ProductFlavor>& flavors) -> std::vector<std::filesystem::path>;

  // Merge manifest files
  static auto merge_manifest_files(
      const std::vector<std::filesystem::path>& base_files, const BuildType& build_type,
      const std::vector<ProductFlavor>& flavors) -> std::vector<std::filesystem::path>;
};

} // namespace horcrux::core
