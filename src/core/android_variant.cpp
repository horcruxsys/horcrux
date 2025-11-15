// Horcrux - Android Build Variant Support Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_variant.h"

#include <algorithm>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>

namespace horcrux::core {

auto BuildVariant::compute_hash() const -> std::string {
  SHA256_CTX sha256_ctx;
  SHA256_Init(&sha256_ctx);

  // Hash variant name
  SHA256_Update(&sha256_ctx, name.data(), name.size());

  // Hash application ID
  SHA256_Update(&sha256_ctx, application_id.data(), application_id.size());

  // Hash version information
  SHA256_Update(&sha256_ctx, &version_code, sizeof(version_code));
  SHA256_Update(&sha256_ctx, version_name.data(), version_name.size());

  // Hash build flags
  SHA256_Update(&sha256_ctx, &debuggable, sizeof(debuggable));
  SHA256_Update(&sha256_ctx, &minify_enabled, sizeof(minify_enabled));
  SHA256_Update(&sha256_ctx, &shrink_resources, sizeof(shrink_resources));

  // Hash source directories (sorted for determinism)
  std::vector<std::string> sorted_sources;
  for (const auto& dir : source_dirs) {
    sorted_sources.push_back(dir.string());
  }
  std::sort(sorted_sources.begin(), sorted_sources.end());
  for (const auto& dir : sorted_sources) {
    SHA256_Update(&sha256_ctx, dir.data(), dir.size());
  }

  // Hash resource directories (sorted for determinism)
  std::vector<std::string> sorted_resources;
  for (const auto& dir : resource_dirs) {
    sorted_resources.push_back(dir.string());
  }
  std::sort(sorted_resources.begin(), sorted_resources.end());
  for (const auto& dir : sorted_resources) {
    SHA256_Update(&sha256_ctx, dir.data(), dir.size());
  }

  // Hash manifest files (sorted for determinism)
  std::vector<std::string> sorted_manifests;
  for (const auto& file : manifest_files) {
    sorted_manifests.push_back(file.string());
  }
  std::sort(sorted_manifests.begin(), sorted_manifests.end());
  for (const auto& file : sorted_manifests) {
    SHA256_Update(&sha256_ctx, file.data(), file.size());
  }

  // Finalize hash
  std::array<unsigned char, SHA256_DIGEST_LENGTH> hash{};
  SHA256_Final(hash.data(), &sha256_ctx);

  // Convert to hex string
  std::ostringstream oss;
  for (unsigned char byte : hash) {
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }

  return oss.str();
}

auto VariantMatrixConfig::validate() const -> tl::expected<void, std::string> {
  // Validate that we have at least one build type
  if (build_types.empty()) {
    return tl::unexpected("At least one build type must be specified");
  }

  // Validate flavor dimensions
  if (!flavor_dimensions.empty()) {
    // Check that all flavors have a valid dimension
    for (const auto& flavor : flavors) {
      bool found = false;
      for (const auto& dim : flavor_dimensions) {
        if (flavor.dimension == dim) {
          found = true;
          break;
        }
      }
      if (!found) {
        return tl::unexpected("Flavor '" + flavor.name + "' has invalid dimension '" +
                             flavor.dimension + "'");
      }
    }

    // Check that each dimension has at least one flavor
    for (const auto& dim : flavor_dimensions) {
      bool found = false;
      for (const auto& flavor : flavors) {
        if (flavor.dimension == dim) {
          found = true;
          break;
        }
      }
      if (!found) {
        return tl::unexpected("Dimension '" + dim + "' has no flavors");
      }
    }
  }

  return {};
}

auto VariantMatrix::find_variant(const std::string& name) const
    -> std::optional<BuildVariant> {
  for (const auto& variant : variants) {
    if (variant.name == name) {
      return variant;
    }
  }
  return std::nullopt;
}

auto VariantMatrix::get_variant_names() const -> std::vector<std::string> {
  std::vector<std::string> names;
  names.reserve(variants.size());
  for (const auto& variant : variants) {
    names.push_back(variant.name);
  }
  return names;
}

auto AndroidVariantBuilder::build_matrix(const VariantMatrixConfig& config)
    -> tl::expected<VariantMatrix, std::string> {
  // Validate configuration
  auto validation = config.validate();
  if (!validation) {
    return tl::unexpected(validation.error());
  }

  // Generate all variants
  auto variants = generate_variants(config);

  VariantMatrix matrix;
  matrix.variants = std::move(variants);

  return matrix;
}

auto AndroidVariantBuilder::generate_variants(const VariantMatrixConfig& config)
    -> std::vector<BuildVariant> {
  std::vector<BuildVariant> variants;

  // If no flavors, just create variants for each build type
  if (config.flavors.empty()) {
    for (const auto& build_type : config.build_types) {
      auto variant = merge_variant_config(config, build_type, {});
      variants.push_back(std::move(variant));
    }
    return variants;
  }

  // Generate all flavor combinations (cartesian product across dimensions)
  std::vector<std::vector<ProductFlavor>> flavor_combinations;

  // Helper function to generate combinations recursively
  std::function<void(size_t, std::vector<ProductFlavor>&)> generate_combinations;
  generate_combinations = [&](size_t dim_index, std::vector<ProductFlavor>& current) {
    if (dim_index >= config.flavor_dimensions.size()) {
      // Base case: we have one flavor per dimension
      flavor_combinations.push_back(current);
      return;
    }

    // Get all flavors for this dimension
    const auto& current_dim = config.flavor_dimensions[dim_index];
    for (const auto& flavor : config.flavors) {
      if (flavor.dimension == current_dim) {
        current.push_back(flavor);
        generate_combinations(dim_index + 1, current);
        current.pop_back();
      }
    }
  };

  std::vector<ProductFlavor> current_combination;
  generate_combinations(0, current_combination);

  // Generate variants for each build type and flavor combination
  for (const auto& build_type : config.build_types) {
    for (const auto& flavor_combo : flavor_combinations) {
      auto variant = merge_variant_config(config, build_type, flavor_combo);
      variants.push_back(std::move(variant));
    }
  }

  return variants;
}

auto AndroidVariantBuilder::merge_variant_config(const VariantMatrixConfig& config,
                                                 const BuildType& build_type,
                                                 const std::vector<ProductFlavor>& flavors)
    -> BuildVariant {
  BuildVariant variant;

  // Generate variant name
  variant.name = generate_variant_name(build_type, flavors);
  variant.build_type = build_type;
  variant.flavors = flavors;

  // Merge application ID
  std::string base_app_id = config.base_application_id;
  
  // Override with flavor application ID if specified
  for (const auto& flavor : flavors) {
    if (flavor.application_id.has_value()) {
      base_app_id = *flavor.application_id;
      break; // Use first flavor that specifies application ID
    }
  }
  
  variant.application_id = merge_application_id(base_app_id, build_type, flavors);

  // Merge version code
  variant.version_code = config.base_version_code;
  for (const auto& flavor : flavors) {
    if (flavor.version_code.has_value()) {
      variant.version_code = *flavor.version_code;
      break; // Use first flavor that specifies version code
    }
  }

  // Merge version name
  variant.version_name = merge_version_name(config.base_version_name, build_type, flavors);

  // Merge SDK versions
  variant.min_sdk = config.base_min_sdk;
  for (const auto& flavor : flavors) {
    if (flavor.min_sdk.has_value()) {
      variant.min_sdk = flavor.min_sdk;
      break; // Use first flavor that specifies min SDK
    }
  }

  variant.target_sdk = config.base_target_sdk;
  for (const auto& flavor : flavors) {
    if (flavor.target_sdk.has_value()) {
      variant.target_sdk = flavor.target_sdk;
      break; // Use first flavor that specifies target SDK
    }
  }

  // Set build flags
  variant.debuggable = build_type.debuggable;
  variant.minify_enabled = build_type.minify_enabled;
  variant.shrink_resources = build_type.shrink_resources;

  // Merge source directories
  variant.source_dirs = merge_source_dirs(config.base_source_dirs, build_type, flavors);

  // Merge resource directories
  variant.resource_dirs = merge_resource_dirs(config.base_resource_dirs, build_type, flavors);

  // Merge manifest files
  variant.manifest_files =
      merge_manifest_files(config.base_manifest_files, build_type, flavors);

  return variant;
}

auto AndroidVariantBuilder::generate_variant_name(const BuildType& build_type,
                                                  const std::vector<ProductFlavor>& flavors)
    -> std::string {
  if (flavors.empty()) {
    return build_type.name;
  }

  std::string name;
  for (size_t i = 0; i < flavors.size(); ++i) {
    std::string flavor_name = flavors[i].name;
    // Capitalize first letter of all flavors except the first one
    if (i > 0 && !flavor_name.empty()) {
      flavor_name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(flavor_name[0])));
    }
    name += flavor_name;
  }

  // Capitalize first letter of build type
  std::string build_type_name = build_type.name;
  if (!build_type_name.empty()) {
    build_type_name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(build_type_name[0])));
  }

  name += build_type_name;
  return name;
}

auto AndroidVariantBuilder::merge_application_id(const std::string& base_id,
                                                 const BuildType& build_type,
                                                 const std::vector<ProductFlavor>& flavors)
    -> std::string {
  std::string result = base_id;

  // Add flavor suffixes
  for (const auto& flavor : flavors) {
    if (flavor.application_id_suffix.has_value()) {
      result += *flavor.application_id_suffix;
    }
  }

  // Add build type suffix
  if (build_type.application_id_suffix.has_value()) {
    result += *build_type.application_id_suffix;
  }

  return result;
}

auto AndroidVariantBuilder::merge_version_name(const std::string& base_version,
                                               const BuildType& build_type,
                                               const std::vector<ProductFlavor>& flavors)
    -> std::string {
  std::string result = base_version;

  // Add flavor version if specified
  for (const auto& flavor : flavors) {
    if (flavor.version_name.has_value()) {
      result = *flavor.version_name;
      break; // Use first flavor that specifies version name
    }
  }

  // Add build type suffix
  if (build_type.version_name_suffix.has_value()) {
    result += *build_type.version_name_suffix;
  }

  return result;
}

auto AndroidVariantBuilder::merge_source_dirs(
    const std::vector<std::filesystem::path>& base_dirs,
    const BuildType& build_type,
    const std::vector<ProductFlavor>& flavors) -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> result = base_dirs;

  // Add flavor source directories
  for (const auto& flavor : flavors) {
    result.insert(result.end(), flavor.source_dirs.begin(), flavor.source_dirs.end());
  }

  // Add build type source directories
  result.insert(result.end(), build_type.source_dirs.begin(), build_type.source_dirs.end());

  return result;
}

auto AndroidVariantBuilder::merge_resource_dirs(
    const std::vector<std::filesystem::path>& base_dirs,
    const BuildType& build_type,
    const std::vector<ProductFlavor>& flavors) -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> result = base_dirs;

  // Add flavor resource directories
  for (const auto& flavor : flavors) {
    result.insert(result.end(), flavor.resource_dirs.begin(), flavor.resource_dirs.end());
  }

  // Add build type resource directories
  result.insert(result.end(), build_type.resource_dirs.begin(),
                build_type.resource_dirs.end());

  return result;
}

auto AndroidVariantBuilder::merge_manifest_files(
    const std::vector<std::filesystem::path>& base_files,
    const BuildType& build_type,
    const std::vector<ProductFlavor>& flavors) -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> result = base_files;

  // Add flavor manifest files
  for (const auto& flavor : flavors) {
    result.insert(result.end(), flavor.manifest_files.begin(), flavor.manifest_files.end());
  }

  // Add build type manifest files
  result.insert(result.end(), build_type.manifest_files.begin(),
                build_type.manifest_files.end());

  return result;
}

} // namespace horcrux::core
