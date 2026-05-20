// Horcrux - Android SDK & NDK Toolchain Detection
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

namespace horcrux::core {

// Error types for Android toolchain detection
enum class AndroidToolchainError {
  SdkNotFound,
  NdkNotFound,
  JavaNotFound,
  InvalidSdkStructure,
  InvalidNdkStructure,
  InvalidJavaHome,
  IoError,
  PermissionDenied,
  UnknownError
};

// Convert error to string
auto to_string(AndroidToolchainError error) -> std::string;

// Android SDK component information
struct AndroidSdkComponent {
  std::string name;
  std::string version;
  std::filesystem::path path;
  std::string hash; // SHA-256 hash for hermetic builds
};

// Android Build Tools information
struct AndroidBuildTools {
  std::string version;
  std::filesystem::path path;
  std::filesystem::path aapt_path;
  std::filesystem::path aapt2_path;
  std::filesystem::path aidl_path;
  std::filesystem::path dx_path;
  std::filesystem::path d8_path;
  std::filesystem::path r8_path;
  std::filesystem::path zipalign_path;
};

// Android Platform information
struct AndroidPlatform {
  std::string api_level;
  std::string version;
  std::filesystem::path path;
  std::filesystem::path android_jar_path;
};

// Android NDK information
struct AndroidNdk {
  std::string version;
  std::filesystem::path path;
  std::filesystem::path toolchain_path;
  std::vector<std::string> supported_abis;
};

// Java SDK information
struct JavaSdk {
  std::string version;
  std::filesystem::path java_home;
  std::filesystem::path javac_path;
  std::filesystem::path java_path;
  std::filesystem::path jar_path;
};

// Complete Android toolchain configuration
struct AndroidToolchain {
  std::filesystem::path sdk_root;
  std::optional<std::filesystem::path> ndk_root;
  std::optional<JavaSdk> java_sdk;

  std::vector<AndroidBuildTools> build_tools;
  std::vector<AndroidPlatform> platforms;
  std::vector<AndroidNdk> ndks;

  std::optional<std::filesystem::path> cmdline_tools;
  std::optional<std::filesystem::path> platform_tools;

  // Hash of entire toolchain configuration for reproducibility
  std::string merkle_hash;

  // Serialize to JSON for .horcrux/lock/android-toolchain.json
  auto to_json() const -> std::string;

  // Deserialize from JSON
  static auto
  from_json(const std::string& json) -> tl::expected<AndroidToolchain, AndroidToolchainError>;
};

// Android toolchain detector
class AndroidToolchainDetector {
public:
  // Detect Android toolchain from environment
  static auto detect() -> tl::expected<AndroidToolchain, AndroidToolchainError>;

  // Detect with custom paths (for testing or explicit configuration)
  static auto detect(const std::filesystem::path& sdk_root,
                     const std::optional<std::filesystem::path>& ndk_root = std::nullopt,
                     const std::optional<std::filesystem::path>& java_home = std::nullopt)
      -> tl::expected<AndroidToolchain, AndroidToolchainError>;

  // Validate an existing toolchain configuration
  static auto
  validate(const AndroidToolchain& toolchain) -> tl::expected<void, AndroidToolchainError>;

private:
  // Detect SDK root from environment variables
  static auto detect_sdk_root() -> std::optional<std::filesystem::path>;

  // Detect NDK root from environment variables or SDK
  static auto
  detect_ndk_root(const std::filesystem::path& sdk_root) -> std::optional<std::filesystem::path>;

  // Detect Java home from environment
  static auto detect_java_home() -> std::optional<std::filesystem::path>;

  // Scan for build tools
  static auto scan_build_tools(const std::filesystem::path& sdk_root)
      -> tl::expected<std::vector<AndroidBuildTools>, AndroidToolchainError>;

  // Scan for platforms
  static auto scan_platforms(const std::filesystem::path& sdk_root)
      -> tl::expected<std::vector<AndroidPlatform>, AndroidToolchainError>;

  // Scan for NDKs
  static auto scan_ndks(const std::filesystem::path& ndk_root)
      -> tl::expected<std::vector<AndroidNdk>, AndroidToolchainError>;

  // Detect Java SDK
  static auto detect_java_sdk(const std::filesystem::path& java_home)
      -> tl::expected<JavaSdk, AndroidToolchainError>;

  // Compute Merkle hash of toolchain
  static auto compute_merkle_hash(const AndroidToolchain& toolchain) -> std::string;
};

// Android toolchain validator
class AndroidToolchainValidator {
public:
  // Validate SDK structure
  static auto validate_sdk(const std::filesystem::path& sdk_root) -> bool;

  // Validate NDK structure
  static auto validate_ndk(const std::filesystem::path& ndk_root) -> bool;

  // Validate Java home
  static auto validate_java_home(const std::filesystem::path& java_home) -> bool;

  // Check if path exists and is readable
  static auto is_readable(const std::filesystem::path& path) -> bool;

  // Check if file is executable
  static auto is_executable(const std::filesystem::path& path) -> bool;
};

} // namespace horcrux::core
