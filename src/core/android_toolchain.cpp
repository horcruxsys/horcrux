// Horcrux - Android SDK & NDK Toolchain Detection Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_toolchain.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <system_error>

#include <nlohmann/json.hpp>

#include "local_cache.h" // For SHA-256 hashing

namespace horcrux::core {

namespace {

// Get environment variable as optional filesystem path
auto get_env_path(const char* var_name) -> std::optional<std::filesystem::path> {
  const char* value = std::getenv(var_name);
  if (value == nullptr || value[0] == '\0') {
    return std::nullopt;
  }
  return std::filesystem::path(value);
}

// Read first line from a file
auto read_first_line(const std::filesystem::path& file_path) -> std::optional<std::string> {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    return std::nullopt;
  }

  std::string line;
  if (std::getline(file, line)) {
    // Trim whitespace
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" \t\r\n") + 1);
    return line;
  }

  return std::nullopt;
}

// Check if directory contains expected subdirectories
auto has_subdirectories(const std::filesystem::path& path,
                        const std::vector<std::string>& expected) -> bool {
  for (const auto& subdir : expected) {
    if (!std::filesystem::exists(path / subdir)) {
      return false;
    }
  }
  return true;
}

// Extract version from directory name (e.g., "34.0.0" from "build-tools/34.0.0")
auto extract_version(const std::string& dir_name) -> std::optional<std::string> {
  std::regex version_regex(R"((\d+(?:\.\d+)*(?:[-.][a-zA-Z0-9]+)?))");
  std::smatch match;
  if (std::regex_search(dir_name, match, version_regex)) {
    return match[1].str();
  }
  return std::nullopt;
}

} // anonymous namespace

auto to_string(AndroidToolchainError error) -> std::string {
  switch (error) {
  case AndroidToolchainError::SdkNotFound:
    return "Android SDK not found. Set ANDROID_HOME or ANDROID_SDK_ROOT environment variable.";
  case AndroidToolchainError::NdkNotFound:
    return "Android NDK not found. Set ANDROID_NDK_ROOT or install NDK in SDK directory.";
  case AndroidToolchainError::JavaNotFound:
    return "Java SDK not found. Set JAVA_HOME environment variable.";
  case AndroidToolchainError::InvalidSdkStructure:
    return "Invalid Android SDK structure. SDK directory is missing required components.";
  case AndroidToolchainError::InvalidNdkStructure:
    return "Invalid Android NDK structure. NDK directory is missing required components.";
  case AndroidToolchainError::InvalidJavaHome:
    return "Invalid JAVA_HOME. Directory does not contain a valid Java installation.";
  case AndroidToolchainError::IoError:
    return "I/O error while accessing toolchain directories.";
  case AndroidToolchainError::PermissionDenied:
    return "Permission denied accessing toolchain directories.";
  case AndroidToolchainError::UnknownError:
    return "Unknown error during toolchain detection.";
  }
  return "Unknown error";
}

auto AndroidToolchain::to_json() const -> std::string {
  nlohmann::json j;

  j["sdk_root"] = sdk_root.string();

  if (ndk_root) {
    j["ndk_root"] = ndk_root->string();
  }

  if (java_sdk) {
    j["java_sdk"]["version"] = java_sdk->version;
    j["java_sdk"]["java_home"] = java_sdk->java_home.string();
  }

  auto& bt_arr = j["build_tools"] = nlohmann::json::array();
  for (const auto& bt : build_tools) {
    nlohmann::json obj;
    obj["version"] = bt.version;
    obj["path"] = bt.path.string();
    bt_arr.push_back(std::move(obj));
  }

  auto& plat_arr = j["platforms"] = nlohmann::json::array();
  for (const auto& p : platforms) {
    nlohmann::json obj;
    obj["api_level"] = p.api_level;
    obj["version"] = p.version;
    obj["path"] = p.path.string();
    plat_arr.push_back(std::move(obj));
  }

  auto& ndk_arr = j["ndks"] = nlohmann::json::array();
  for (const auto& ndk : ndks) {
    nlohmann::json obj;
    obj["version"] = ndk.version;
    obj["path"] = ndk.path.string();
    ndk_arr.push_back(std::move(obj));
  }

  j["merkle_hash"] = merkle_hash;

  return j.dump(2);
}

auto AndroidToolchain::from_json(const std::string& json_str)
    -> tl::expected<AndroidToolchain, AndroidToolchainError> {
  nlohmann::json j;
  try {
    j = nlohmann::json::parse(json_str);
  } catch (const nlohmann::json::parse_error&) {
    return tl::unexpected(AndroidToolchainError::UnknownError);
  }

  AndroidToolchain tc;
  tc.sdk_root = j.value("sdk_root", std::string{});

  if (auto it = j.find("ndk_root"); it != j.end() && !it->is_null()) {
    tc.ndk_root = it->get<std::string>();
  }

  if (auto it = j.find("java_sdk"); it != j.end() && it->is_object()) {
    JavaSdk js;
    js.version = it->value("version", std::string{});
    js.java_home = it->value("java_home", std::string{});
    tc.java_sdk = std::move(js);
  }

  if (auto bt_it = j.find("build_tools"); bt_it != j.end() && bt_it->is_array()) {
    for (const auto& obj : *bt_it) {
      AndroidBuildTools bt;
      bt.version = obj.value("version", std::string{});
      bt.path = obj.value("path", std::string{});
      tc.build_tools.push_back(std::move(bt));
    }
  }

  if (auto plat_it = j.find("platforms"); plat_it != j.end() && plat_it->is_array()) {
    for (const auto& obj : *plat_it) {
      AndroidPlatform plat;
      plat.api_level = obj.value("api_level", std::string{});
      plat.version = obj.value("version", std::string{});
      plat.path = obj.value("path", std::string{});
      tc.platforms.push_back(std::move(plat));
    }
  }

  if (auto ndk_it = j.find("ndks"); ndk_it != j.end() && ndk_it->is_array()) {
    for (const auto& obj : *ndk_it) {
      AndroidNdk ndk;
      ndk.version = obj.value("version", std::string{});
      ndk.path = obj.value("path", std::string{});
      tc.ndks.push_back(std::move(ndk));
    }
  }

  tc.merkle_hash = j.value("merkle_hash", std::string{});

  return tc;
}

auto AndroidToolchainDetector::detect() -> tl::expected<AndroidToolchain, AndroidToolchainError> {
  // Detect SDK root
  auto sdk_root = detect_sdk_root();
  if (!sdk_root) {
    return tl::unexpected(AndroidToolchainError::SdkNotFound);
  }

  // Detect NDK root
  auto ndk_root = detect_ndk_root(*sdk_root);

  // Detect Java home
  auto java_home = detect_java_home();

  return detect(*sdk_root, ndk_root, java_home);
}

auto AndroidToolchainDetector::detect(const std::filesystem::path& sdk_root,
                                      const std::optional<std::filesystem::path>& ndk_root,
                                      const std::optional<std::filesystem::path>& java_home)
    -> tl::expected<AndroidToolchain, AndroidToolchainError> {
  AndroidToolchain toolchain;
  toolchain.sdk_root = sdk_root;
  toolchain.ndk_root = ndk_root;

  // Validate SDK structure
  if (!AndroidToolchainValidator::validate_sdk(sdk_root)) {
    return tl::unexpected(AndroidToolchainError::InvalidSdkStructure);
  }

  // Scan build tools
  auto build_tools_result = scan_build_tools(sdk_root);
  if (!build_tools_result) {
    return tl::unexpected(build_tools_result.error());
  }
  toolchain.build_tools = std::move(*build_tools_result);

  // Scan platforms
  auto platforms_result = scan_platforms(sdk_root);
  if (!platforms_result) {
    return tl::unexpected(platforms_result.error());
  }
  toolchain.platforms = std::move(*platforms_result);

  // Scan NDKs if NDK root is available
  if (ndk_root) {
    auto ndks_result = scan_ndks(*ndk_root);
    if (!ndks_result) {
      return tl::unexpected(ndks_result.error());
    }
    toolchain.ndks = std::move(*ndks_result);
  }

  // Detect Java SDK if Java home is available
  if (java_home) {
    auto java_sdk_result = detect_java_sdk(*java_home);
    if (java_sdk_result) {
      toolchain.java_sdk = std::move(*java_sdk_result);
    }
  }

  // Detect cmdline-tools
  auto cmdline_tools_path = sdk_root / "cmdline-tools";
  if (std::filesystem::exists(cmdline_tools_path)) {
    toolchain.cmdline_tools = cmdline_tools_path;
  }

  // Detect platform-tools
  auto platform_tools_path = sdk_root / "platform-tools";
  if (std::filesystem::exists(platform_tools_path)) {
    toolchain.platform_tools = platform_tools_path;
  }

  // Compute Merkle hash
  toolchain.merkle_hash = compute_merkle_hash(toolchain);

  return toolchain;
}

auto AndroidToolchainDetector::validate(const AndroidToolchain& toolchain)
    -> tl::expected<void, AndroidToolchainError> {
  if (!AndroidToolchainValidator::validate_sdk(toolchain.sdk_root)) {
    return tl::unexpected(AndroidToolchainError::InvalidSdkStructure);
  }

  if (toolchain.ndk_root && !AndroidToolchainValidator::validate_ndk(*toolchain.ndk_root)) {
    return tl::unexpected(AndroidToolchainError::InvalidNdkStructure);
  }

  if (toolchain.java_sdk &&
      !AndroidToolchainValidator::validate_java_home(toolchain.java_sdk->java_home)) {
    return tl::unexpected(AndroidToolchainError::InvalidJavaHome);
  }

  return {};
}

auto AndroidToolchainDetector::detect_sdk_root() -> std::optional<std::filesystem::path> {
  // Try ANDROID_HOME first
  if (auto path = get_env_path("ANDROID_HOME")) {
    if (std::filesystem::exists(*path)) {
      return path;
    }
  }

  // Try ANDROID_SDK_ROOT
  if (auto path = get_env_path("ANDROID_SDK_ROOT")) {
    if (std::filesystem::exists(*path)) {
      return path;
    }
  }

  // Try common installation locations
  std::vector<std::filesystem::path> common_paths = {
      std::filesystem::path(std::getenv("HOME") ? std::getenv("HOME") : "") / "Android/Sdk",
      std::filesystem::path(std::getenv("HOME") ? std::getenv("HOME") : "") / "Library/Android/sdk",
      "/usr/local/android-sdk", "/opt/android-sdk"};

  for (const auto& path : common_paths) {
    if (std::filesystem::exists(path) && AndroidToolchainValidator::validate_sdk(path)) {
      return path;
    }
  }

  return std::nullopt;
}

auto AndroidToolchainDetector::detect_ndk_root(const std::filesystem::path& sdk_root)
    -> std::optional<std::filesystem::path> {
  // Try ANDROID_NDK_ROOT first
  if (auto path = get_env_path("ANDROID_NDK_ROOT")) {
    if (std::filesystem::exists(*path)) {
      return path;
    }
  }

  // Try ANDROID_NDK_HOME
  if (auto path = get_env_path("ANDROID_NDK_HOME")) {
    if (std::filesystem::exists(*path)) {
      return path;
    }
  }

  // Try NDK in SDK directory
  auto ndk_bundle = sdk_root / "ndk-bundle";
  if (std::filesystem::exists(ndk_bundle)) {
    return ndk_bundle;
  }

  // Try versioned NDK directory
  auto ndk_dir = sdk_root / "ndk";
  if (std::filesystem::exists(ndk_dir)) {
    // Return the ndk directory, scan_ndks will find versions inside
    return ndk_dir;
  }

  return std::nullopt;
}

auto AndroidToolchainDetector::detect_java_home() -> std::optional<std::filesystem::path> {
  // Try JAVA_HOME
  if (auto path = get_env_path("JAVA_HOME")) {
    if (std::filesystem::exists(*path)) {
      return path;
    }
  }

  // Try to find java executable and derive JAVA_HOME
  // This is a fallback and may not always work correctly
  return std::nullopt;
}

auto AndroidToolchainDetector::scan_build_tools(const std::filesystem::path& sdk_root)
    -> tl::expected<std::vector<AndroidBuildTools>, AndroidToolchainError> {
  std::vector<AndroidBuildTools> build_tools;
  auto build_tools_dir = sdk_root / "build-tools";

  if (!std::filesystem::exists(build_tools_dir)) {
    return build_tools; // Empty list is valid
  }

  try {
    for (const auto& entry : std::filesystem::directory_iterator(build_tools_dir)) {
      if (!entry.is_directory()) {
        continue;
      }

      auto version = extract_version(entry.path().filename().string());
      if (!version) {
        continue;
      }

      AndroidBuildTools bt;
      bt.version = *version;
      bt.path = entry.path();

      // Detect tool paths
#ifdef _WIN32
      const std::string exe_ext = ".exe";
#else
      const std::string exe_ext = "";
#endif

      bt.aapt_path = bt.path / ("aapt" + exe_ext);
      bt.aapt2_path = bt.path / ("aapt2" + exe_ext);
      bt.aidl_path = bt.path / ("aidl" + exe_ext);
      bt.dx_path = bt.path / ("dx" + exe_ext);
      bt.d8_path = bt.path / ("d8" + exe_ext);
      bt.r8_path = bt.path / ("r8" + exe_ext);
      bt.zipalign_path = bt.path / ("zipalign" + exe_ext);

      build_tools.push_back(std::move(bt));
    }
  } catch (const std::filesystem::filesystem_error&) {
    return tl::unexpected(AndroidToolchainError::IoError);
  }

  // Sort by version (descending)
  std::sort(
      build_tools.begin(), build_tools.end(),
      [](const AndroidBuildTools& a, const AndroidBuildTools& b) { return a.version > b.version; });

  return build_tools;
}

auto AndroidToolchainDetector::scan_platforms(const std::filesystem::path& sdk_root)
    -> tl::expected<std::vector<AndroidPlatform>, AndroidToolchainError> {
  std::vector<AndroidPlatform> platforms;
  auto platforms_dir = sdk_root / "platforms";

  if (!std::filesystem::exists(platforms_dir)) {
    return platforms; // Empty list is valid
  }

  try {
    for (const auto& entry : std::filesystem::directory_iterator(platforms_dir)) {
      if (!entry.is_directory()) {
        continue;
      }

      std::string dir_name = entry.path().filename().string();

      // Extract API level (e.g., "android-33" -> "33")
      std::regex api_regex(R"(android-(\d+))");
      std::smatch match;
      if (!std::regex_match(dir_name, match, api_regex)) {
        continue;
      }

      AndroidPlatform platform;
      platform.api_level = match[1].str();
      platform.path = entry.path();
      platform.android_jar_path = platform.path / "android.jar";

      // Try to read version from source.properties
      auto source_props = platform.path / "source.properties";
      if (std::filesystem::exists(source_props)) {
        if (auto version = read_first_line(source_props)) {
          platform.version = *version;
        }
      }

      if (platform.version.empty()) {
        platform.version = platform.api_level;
      }

      platforms.push_back(std::move(platform));
    }
  } catch (const std::filesystem::filesystem_error&) {
    return tl::unexpected(AndroidToolchainError::IoError);
  }

  // Sort by API level (descending)
  std::sort(platforms.begin(), platforms.end(),
            [](const AndroidPlatform& a, const AndroidPlatform& b) {
              return std::stoi(a.api_level) > std::stoi(b.api_level);
            });

  return platforms;
}

auto AndroidToolchainDetector::scan_ndks(const std::filesystem::path& ndk_root)
    -> tl::expected<std::vector<AndroidNdk>, AndroidToolchainError> {
  std::vector<AndroidNdk> ndks;

  if (!std::filesystem::exists(ndk_root)) {
    return ndks;
  }

  try {
    // Check if ndk_root itself is a valid NDK (for ndk-bundle)
    auto source_props = ndk_root / "source.properties";
    if (std::filesystem::exists(source_props)) {
      AndroidNdk ndk;
      ndk.path = ndk_root;

      // Read version from source.properties
      std::ifstream props_file(source_props);
      std::string line;
      while (std::getline(props_file, line)) {
        if (line.find("Pkg.Revision") != std::string::npos) {
          auto pos = line.find('=');
          if (pos != std::string::npos) {
            ndk.version = line.substr(pos + 1);
            // Trim whitespace
            ndk.version.erase(0, ndk.version.find_first_not_of(" \t\r\n"));
            ndk.version.erase(ndk.version.find_last_not_of(" \t\r\n") + 1);
          }
        }
      }

      if (ndk.version.empty()) {
        ndk.version = "unknown";
      }

      ndk.toolchain_path = ndk.path / "toolchains";
      ndk.supported_abis = {"armeabi-v7a", "arm64-v8a", "x86", "x86_64"};

      ndks.push_back(std::move(ndk));
      return ndks;
    }

    // Otherwise, scan for versioned NDK directories
    for (const auto& entry : std::filesystem::directory_iterator(ndk_root)) {
      if (!entry.is_directory()) {
        continue;
      }

      source_props = entry.path() / "source.properties";
      if (!std::filesystem::exists(source_props)) {
        continue;
      }

      AndroidNdk ndk;
      ndk.path = entry.path();
      ndk.version = entry.path().filename().string();
      ndk.toolchain_path = ndk.path / "toolchains";
      ndk.supported_abis = {"armeabi-v7a", "arm64-v8a", "x86", "x86_64"};

      ndks.push_back(std::move(ndk));
    }
  } catch (const std::filesystem::filesystem_error&) {
    return tl::unexpected(AndroidToolchainError::IoError);
  }

  // Sort by version (descending)
  std::sort(ndks.begin(), ndks.end(),
            [](const AndroidNdk& a, const AndroidNdk& b) { return a.version > b.version; });

  return ndks;
}

auto AndroidToolchainDetector::detect_java_sdk(const std::filesystem::path& java_home)
    -> tl::expected<JavaSdk, AndroidToolchainError> {
  if (!AndroidToolchainValidator::validate_java_home(java_home)) {
    return tl::unexpected(AndroidToolchainError::InvalidJavaHome);
  }

  JavaSdk java_sdk;
  java_sdk.java_home = java_home;

#ifdef _WIN32
  const std::string exe_ext = ".exe";
#else
  const std::string exe_ext = "";
#endif

  java_sdk.javac_path = java_home / "bin" / ("javac" + exe_ext);
  java_sdk.java_path = java_home / "bin" / ("java" + exe_ext);
  java_sdk.jar_path = java_home / "bin" / ("jar" + exe_ext);

  // Try to detect Java version
  // This is a simplified version detection
  auto release_file = java_home / "release";
  if (std::filesystem::exists(release_file)) {
    std::ifstream release(release_file);
    std::string line;
    while (std::getline(release, line)) {
      if (line.find("JAVA_VERSION") != std::string::npos) {
        auto pos = line.find('=');
        if (pos != std::string::npos) {
          java_sdk.version = line.substr(pos + 1);
          // Remove quotes
          java_sdk.version.erase(std::remove(java_sdk.version.begin(), java_sdk.version.end(), '"'),
                                 java_sdk.version.end());
        }
      }
    }
  }

  if (java_sdk.version.empty()) {
    java_sdk.version = "unknown";
  }

  return java_sdk;
}

auto AndroidToolchainDetector::compute_merkle_hash(const AndroidToolchain& toolchain)
    -> std::string {
  // Create a deterministic string representation of the toolchain
  std::ostringstream ss;

  ss << "sdk_root:" << toolchain.sdk_root.string() << "\n";

  if (toolchain.ndk_root) {
    ss << "ndk_root:" << toolchain.ndk_root->string() << "\n";
  }

  if (toolchain.java_sdk) {
    ss << "java_version:" << toolchain.java_sdk->version << "\n";
    ss << "java_home:" << toolchain.java_sdk->java_home.string() << "\n";
  }

  for (const auto& bt : toolchain.build_tools) {
    ss << "build_tools:" << bt.version << ":" << bt.path.string() << "\n";
  }

  for (const auto& p : toolchain.platforms) {
    ss << "platform:" << p.api_level << ":" << p.path.string() << "\n";
  }

  for (const auto& ndk : toolchain.ndks) {
    ss << "ndk:" << ndk.version << ":" << ndk.path.string() << "\n";
  }

  // Compute SHA-256 hash
  std::string data_str = ss.str();
  std::vector<uint8_t> data(data_str.begin(), data_str.end());
  auto hash = compute_sha256(data);
  return hash_to_string(hash);
}

// Validator implementations

auto AndroidToolchainValidator::validate_sdk(const std::filesystem::path& sdk_root) -> bool {
  if (!std::filesystem::exists(sdk_root)) {
    return false;
  }

  // Check for required subdirectories (at least one should exist)
  std::vector<std::string> required = {"platforms", "build-tools"};

  for (const auto& subdir : required) {
    if (std::filesystem::exists(sdk_root / subdir)) {
      return true; // At least one required directory exists
    }
  }

  return false;
}

auto AndroidToolchainValidator::validate_ndk(const std::filesystem::path& ndk_root) -> bool {
  if (!std::filesystem::exists(ndk_root)) {
    return false;
  }

  // Check for NDK structure
  auto source_props = ndk_root / "source.properties";
  if (std::filesystem::exists(source_props)) {
    return true;
  }

  // Check if it's a versioned NDK directory
  try {
    for (const auto& entry : std::filesystem::directory_iterator(ndk_root)) {
      if (entry.is_directory()) {
        auto sub_source_props = entry.path() / "source.properties";
        if (std::filesystem::exists(sub_source_props)) {
          return true;
        }
      }
    }
  } catch (const std::filesystem::filesystem_error&) {
    return false;
  }

  return false;
}

auto AndroidToolchainValidator::validate_java_home(const std::filesystem::path& java_home) -> bool {
  if (!std::filesystem::exists(java_home)) {
    return false;
  }

  // Check for bin directory and java executable
#ifdef _WIN32
  auto java_exe = java_home / "bin" / "java.exe";
#else
  auto java_exe = java_home / "bin" / "java";
#endif

  return std::filesystem::exists(java_exe);
}

auto AndroidToolchainValidator::is_readable(const std::filesystem::path& path) -> bool {
  try {
    std::error_code ec;
    auto perms = std::filesystem::status(path, ec).permissions();
    if (ec) {
      return false;
    }

    using perms_type = std::filesystem::perms;
    return (perms & perms_type::owner_read) != perms_type::none ||
           (perms & perms_type::group_read) != perms_type::none ||
           (perms & perms_type::others_read) != perms_type::none;
  } catch (...) {
    return false;
  }
}

auto AndroidToolchainValidator::is_executable(const std::filesystem::path& path) -> bool {
  try {
    std::error_code ec;
    auto perms = std::filesystem::status(path, ec).permissions();
    if (ec) {
      return false;
    }

    using perms_type = std::filesystem::perms;
    return (perms & perms_type::owner_exec) != perms_type::none ||
           (perms & perms_type::group_exec) != perms_type::none ||
           (perms & perms_type::others_exec) != perms_type::none;
  } catch (...) {
    return false;
  }
}

} // namespace horcrux::core
