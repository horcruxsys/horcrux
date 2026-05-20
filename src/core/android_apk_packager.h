// Horcrux - Android APK Packaging
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "android_toolchain.h"

namespace horcrux::core {

// Error types for Android APK packaging
enum class AndroidApkPackagerError {
  InvalidConfiguration,
  ZipalignNotFound,
  ApksignerNotFound,
  SigningFailed,
  VerificationFailed,
  PackagingFailed,
  IoError,
  UnknownError
};

// Convert error to string
auto to_string(AndroidApkPackagerError error) -> std::string;

// APK signing configuration
struct ApkSigningConfig {
  std::filesystem::path keystore_path;
  std::string keystore_password;
  std::string key_alias;
  std::string key_password;
  std::optional<std::string> v1_signing_enabled; // JAR signing
  std::optional<std::string> v2_signing_enabled; // APK Signature Scheme v2
  std::optional<std::string> v3_signing_enabled; // APK Signature Scheme v3
  std::optional<std::string> v4_signing_enabled; // APK Signature Scheme v4
};

// APK packaging configuration
struct ApkPackagingConfig {
  std::filesystem::path resources_apk;            // resources.ap_ from AAPT2
  std::vector<std::filesystem::path> dex_files;   // classes.dex, classes2.dex, etc.
  std::vector<std::filesystem::path> native_libs; // .so files
  std::vector<std::filesystem::path> assets;      // Asset files
  std::filesystem::path output_apk;               // Output APK path
  std::optional<ApkSigningConfig> signing_config; // Signing configuration
  bool zipalign = true;                           // Enable zipalign
  int zipalign_alignment = 4;                     // Alignment in bytes (4 or 8)
  bool verify_signature = true;                   // Verify signature after signing
  bool verbose = false;                           // Enable verbose output
  std::vector<std::string> additional_args;       // Additional arguments
};

// APK packaging result
struct ApkPackagingResult {
  std::filesystem::path output_apk;
  bool is_signed;
  bool is_verified;
  std::chrono::milliseconds packaging_time;
  std::string packaging_hash;
};

// APK verification result
struct ApkVerificationResult {
  bool is_valid;
  std::vector<std::string> signatures; // List of signature algorithms used
  std::optional<std::string> error_message;
};

// Main Android APK packager class
class AndroidApkPackager {
public:
  explicit AndroidApkPackager(const AndroidToolchain& toolchain);

  // Package APK: resources + DEX + native libs + assets -> final APK
  auto package(const ApkPackagingConfig& config)
      -> tl::expected<ApkPackagingResult, AndroidApkPackagerError>;

  // Zipalign APK: align APK file boundaries
  auto zipalign(const std::filesystem::path& input_apk, const std::filesystem::path& output_apk,
                int alignment = 4) -> tl::expected<void, AndroidApkPackagerError>;

  // Sign APK: add signature to APK
  auto sign(const std::filesystem::path& input_apk, const std::filesystem::path& output_apk,
            const ApkSigningConfig& signing_config) -> tl::expected<void, AndroidApkPackagerError>;

  // Verify APK signature
  auto verify(const std::filesystem::path& apk_path)
      -> tl::expected<ApkVerificationResult, AndroidApkPackagerError>;

  // Get zipalign path from toolchain
  auto get_zipalign_path() const -> std::optional<std::filesystem::path>;

  // Get apksigner path from toolchain
  auto get_apksigner_path() const -> std::optional<std::filesystem::path>;

  // Validate configuration
  static auto
  validate_config(const ApkPackagingConfig& config) -> tl::expected<void, AndroidApkPackagerError>;

  // Compute packaging hash (Merkle signature)
  static auto compute_packaging_hash(const ApkPackagingConfig& config) -> std::string;

private:
  const AndroidToolchain& toolchain_;
  std::optional<std::filesystem::path> zipalign_path_;
  std::optional<std::filesystem::path> apksigner_path_;

  // Find zipalign in build tools
  auto find_zipalign() const -> std::optional<std::filesystem::path>;

  // Find apksigner in build tools
  auto find_apksigner() const -> std::optional<std::filesystem::path>;

  // Execute command and capture output
  auto execute_command(const std::vector<std::string>& args,
                       const std::optional<std::filesystem::path>& working_dir = std::nullopt)
      -> tl::expected<std::string, AndroidApkPackagerError>;

  // Create temporary working directory
  auto create_temp_dir() const -> std::filesystem::path;

  // Package APK using zip (combines all components)
  auto package_apk_internal(const ApkPackagingConfig& config, const std::filesystem::path& temp_dir)
      -> tl::expected<std::filesystem::path, AndroidApkPackagerError>;
};

// Utility namespace for APK operations
namespace apk_utils {

// Check if APK is aligned
auto is_aligned(const std::filesystem::path& apk_path, int alignment = 4) -> bool;

// Extract APK to directory
auto extract_apk(const std::filesystem::path& apk_path, const std::filesystem::path& output_dir)
    -> tl::expected<void, AndroidApkPackagerError>;

// Get APK information
struct ApkInfo {
  std::string package_name;
  std::string version_name;
  int version_code;
  int min_sdk_version;
  int target_sdk_version;
  std::vector<std::string> permissions;
  std::vector<std::string> features;
};

auto get_apk_info(const std::filesystem::path& apk_path)
    -> tl::expected<ApkInfo, AndroidApkPackagerError>;

} // namespace apk_utils

} // namespace horcrux::core
