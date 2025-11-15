// Horcrux - Android App Bundle (AAB) Packaging
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#ifndef HORCRUX_CORE_ANDROID_AAB_PACKAGER_H_
#define HORCRUX_CORE_ANDROID_AAB_PACKAGER_H_

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "android_toolchain.h"

namespace horcrux::core {

// Error types for Android AAB packaging
enum class AndroidAabPackagerError {
  InvalidConfiguration,
  BundletoolNotFound,
  PackagingFailed,
  ModuleCreationFailed,
  UniversalApkFailed,
  IoError,
  UnknownError
};

// Convert error to string
auto to_string(AndroidAabPackagerError error) -> std::string;

// AAB module configuration
struct AabModuleConfig {
  std::string module_name;                        // Module name (e.g., "base", "feature1")
  std::filesystem::path manifest;                 // AndroidManifest.xml
  std::filesystem::path resources_apk;            // resources.ap_ from AAPT2
  std::vector<std::filesystem::path> dex_files;   // DEX files
  std::vector<std::filesystem::path> native_libs; // Native libraries
  std::vector<std::filesystem::path> assets;      // Assets
  bool is_base_module = false;                    // Is this the base module?
};

// AAB packaging configuration
struct AabPackagingConfig {
  std::vector<AabModuleConfig> modules;                // Modules (base + features)
  std::filesystem::path output_aab;                    // Output AAB path
  std::optional<std::filesystem::path> bundletool_jar; // bundletool.jar path
  bool verbose = false;                                // Enable verbose output
  std::vector<std::string> additional_args;            // Additional arguments
};

// AAB packaging result
struct AabPackagingResult {
  std::filesystem::path output_aab;
  std::vector<std::string> module_names;
  std::chrono::milliseconds packaging_time;
  std::string packaging_hash;
};

// Universal APK generation configuration
struct UniversalApkConfig {
  std::filesystem::path aab_path;                      // Input AAB
  std::filesystem::path output_apk;                    // Output universal APK
  std::optional<std::filesystem::path> bundletool_jar; // bundletool.jar path
  std::optional<std::filesystem::path> keystore_path;  // Keystore for signing
  std::optional<std::string> keystore_password;
  std::optional<std::string> key_alias;
  std::optional<std::string> key_password;
  bool verbose = false;
};

// Universal APK generation result
struct UniversalApkResult {
  std::filesystem::path output_apk;
  std::chrono::milliseconds generation_time;
};

// Main Android AAB packager class
class AndroidAabPackager {
public:
  explicit AndroidAabPackager(const AndroidToolchain& toolchain);

  // Package AAB: modules -> Android App Bundle
  auto package(const AabPackagingConfig& config)
      -> tl::expected<AabPackagingResult, AndroidAabPackagerError>;

  // Generate universal APK from AAB
  auto generate_universal_apk(const UniversalApkConfig& config)
      -> tl::expected<UniversalApkResult, AndroidAabPackagerError>;

  // Get bundletool path
  auto get_bundletool_path() const -> std::optional<std::filesystem::path>;

  // Download bundletool if not present
  auto download_bundletool(const std::filesystem::path& output_path)
      -> tl::expected<void, AndroidAabPackagerError>;

  // Validate configuration
  static auto
  validate_config(const AabPackagingConfig& config) -> tl::expected<void, AndroidAabPackagerError>;

  // Compute packaging hash
  static auto compute_packaging_hash(const AabPackagingConfig& config) -> std::string;

private:
  const AndroidToolchain& toolchain_;
  std::optional<std::filesystem::path> bundletool_path_;

  // Find bundletool in common locations
  auto find_bundletool() const -> std::optional<std::filesystem::path>;

  // Execute bundletool command
  auto execute_bundletool(const std::vector<std::string>& args)
      -> tl::expected<std::string, AndroidAabPackagerError>;

  // Create module ZIP
  auto create_module_zip(const AabModuleConfig& module, const std::filesystem::path& output_zip)
      -> tl::expected<void, AndroidAabPackagerError>;

  // Execute command
  auto execute_command(const std::vector<std::string>& args)
      -> tl::expected<std::string, AndroidAabPackagerError>;

  // Create temporary working directory
  auto create_temp_dir() const -> std::filesystem::path;
};

// Utility namespace for AAB operations
namespace aab_utils {

// Get AAB information
struct AabInfo {
  std::vector<std::string> module_names;
  std::string package_name;
  std::string version_name;
  int version_code;
  int min_sdk_version;
  int target_sdk_version;
};

auto get_aab_info(const std::filesystem::path& aab_path)
    -> tl::expected<AabInfo, AndroidAabPackagerError>;

// Validate AAB structure
auto validate_aab(const std::filesystem::path& aab_path)
    -> tl::expected<bool, AndroidAabPackagerError>;

// Extract AAB to directory
auto extract_aab(const std::filesystem::path& aab_path, const std::filesystem::path& output_dir)
    -> tl::expected<void, AndroidAabPackagerError>;

} // namespace aab_utils

} // namespace horcrux::core

#endif // HORCRUX_CORE_ANDROID_AAB_PACKAGER_H_
