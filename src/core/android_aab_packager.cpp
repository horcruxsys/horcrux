// Horcrux - Android App Bundle (AAB) Packaging Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_aab_packager.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unistd.h>

#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/wait.h>

namespace horcrux::core {

// Convert error to string
auto to_string(AndroidAabPackagerError error) -> std::string {
  switch (error) {
  case AndroidAabPackagerError::InvalidConfiguration:
    return "Invalid configuration";
  case AndroidAabPackagerError::BundletoolNotFound:
    return "bundletool not found";
  case AndroidAabPackagerError::PackagingFailed:
    return "AAB packaging failed";
  case AndroidAabPackagerError::ModuleCreationFailed:
    return "Module creation failed";
  case AndroidAabPackagerError::UniversalApkFailed:
    return "Universal APK generation failed";
  case AndroidAabPackagerError::IoError:
    return "I/O error";
  case AndroidAabPackagerError::UnknownError:
    return "Unknown error";
  }
  return "Unknown error";
}

// AndroidAabPackager implementation
AndroidAabPackager::AndroidAabPackager(const AndroidToolchain& toolchain)
    : toolchain_(toolchain), bundletool_path_(find_bundletool()) {
}

auto AndroidAabPackager::package(const AabPackagingConfig& config)
    -> tl::expected<AabPackagingResult, AndroidAabPackagerError> {
  auto start_time = std::chrono::steady_clock::now();

  // Validate configuration
  if (auto validation = validate_config(config); !validation) {
    return tl::unexpected(validation.error());
  }

  // Create temporary working directory
  auto temp_dir = create_temp_dir();

  // Create module ZIPs
  std::vector<std::filesystem::path> module_zips;
  std::vector<std::string> module_names;

  for (const auto& module : config.modules) {
    auto module_zip = temp_dir / (module.module_name + ".zip");
    if (auto result = create_module_zip(module, module_zip); !result) {
      return tl::unexpected(result.error());
    }
    module_zips.push_back(module_zip);
    module_names.push_back(module.module_name);
  }

  // Use bundletool to create AAB
  if (!bundletool_path_ || !std::filesystem::exists(*bundletool_path_)) {
    return tl::unexpected(AndroidAabPackagerError::BundletoolNotFound);
  }

  std::vector<std::string> args;
  args.push_back("java");
  args.push_back("-jar");
  args.push_back(bundletool_path_->string());
  args.push_back("build-bundle");

  // Add modules
  for (const auto& module_zip : module_zips) {
    args.push_back("--modules=" + module_zip.string());
  }

  args.push_back("--output=" + config.output_aab.string());

  if (config.verbose) {
    args.push_back("--verbose");
  }

  auto result = execute_bundletool(args);
  if (!result) {
    return tl::unexpected(result.error());
  }

  // Clean up temporary directory
  std::filesystem::remove_all(temp_dir);

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  AabPackagingResult packaging_result;
  packaging_result.output_aab = config.output_aab;
  packaging_result.module_names = module_names;
  packaging_result.packaging_time = duration;
  packaging_result.packaging_hash = compute_packaging_hash(config);

  return packaging_result;
}

auto AndroidAabPackager::generate_universal_apk(const UniversalApkConfig& config)
    -> tl::expected<UniversalApkResult, AndroidAabPackagerError> {
  auto start_time = std::chrono::steady_clock::now();

  if (!std::filesystem::exists(config.aab_path)) {
    return tl::unexpected(AndroidAabPackagerError::InvalidConfiguration);
  }

  auto bundletool =
      config.bundletool_jar.value_or(bundletool_path_.value_or(std::filesystem::path()));

  if (!std::filesystem::exists(bundletool)) {
    return tl::unexpected(AndroidAabPackagerError::BundletoolNotFound);
  }

  // Create temp directory for APKS file
  auto temp_dir = create_temp_dir();
  auto apks_file = temp_dir / "universal.apks";

  std::vector<std::string> args;
  args.push_back("java");
  args.push_back("-jar");
  args.push_back(bundletool.string());
  args.push_back("build-apks");
  args.push_back("--bundle=" + config.aab_path.string());
  args.push_back("--output=" + apks_file.string());
  args.push_back("--mode=universal");

  // Add signing if provided
  if (config.keystore_path && std::filesystem::exists(*config.keystore_path)) {
    args.push_back("--ks=" + config.keystore_path->string());
    if (config.keystore_password) {
      args.push_back("--ks-pass=pass:" + *config.keystore_password);
    }
    if (config.key_alias) {
      args.push_back("--ks-key-alias=" + *config.key_alias);
    }
    if (config.key_password) {
      args.push_back("--key-pass=pass:" + *config.key_password);
    }
  }

  if (config.verbose) {
    args.push_back("--verbose");
  }

  auto result = execute_bundletool(args);
  if (!result) {
    return tl::unexpected(AndroidAabPackagerError::UniversalApkFailed);
  }

  // Extract universal APK from APKS file
  auto extract_dir = temp_dir / "extracted";
  std::filesystem::create_directories(extract_dir);

  std::vector<std::string> unzip_args;
  unzip_args.push_back("unzip");
  unzip_args.push_back("-q");
  unzip_args.push_back("-o");
  unzip_args.push_back(apks_file.string());
  unzip_args.push_back("-d");
  unzip_args.push_back(extract_dir.string());

  auto unzip_result = execute_command(unzip_args);
  if (!unzip_result) {
    return tl::unexpected(unzip_result.error());
  }

  // Find universal.apk in extracted directory
  auto universal_apk = extract_dir / "universal.apk";
  if (!std::filesystem::exists(universal_apk)) {
    return tl::unexpected(AndroidAabPackagerError::UniversalApkFailed);
  }

  // Copy to output location
  std::filesystem::create_directories(config.output_apk.parent_path());
  std::filesystem::copy_file(universal_apk, config.output_apk,
                             std::filesystem::copy_options::overwrite_existing);

  // Clean up
  std::filesystem::remove_all(temp_dir);

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  UniversalApkResult apk_result;
  apk_result.output_apk = config.output_apk;
  apk_result.generation_time = duration;

  return apk_result;
}

auto AndroidAabPackager::get_bundletool_path() const -> std::optional<std::filesystem::path> {
  return bundletool_path_;
}

auto AndroidAabPackager::download_bundletool(const std::filesystem::path& output_path)
    -> tl::expected<void, AndroidAabPackagerError> {
  // Download bundletool from GitHub releases
  // This is a placeholder - in production, you'd use curl or wget
  const std::string bundletool_url =
      "https://github.com/google/bundletool/releases/latest/download/bundletool-all.jar";

  std::vector<std::string> args;
  args.push_back("curl");
  args.push_back("-L");
  args.push_back("-o");
  args.push_back(output_path.string());
  args.push_back(bundletool_url);

  auto result = execute_command(args);
  if (!result) {
    return tl::unexpected(AndroidAabPackagerError::IoError);
  }

  return {};
}

auto AndroidAabPackager::validate_config(const AabPackagingConfig& config)
    -> tl::expected<void, AndroidAabPackagerError> {
  // Check at least one module
  if (config.modules.empty()) {
    return tl::unexpected(AndroidAabPackagerError::InvalidConfiguration);
  }

  // Check that exactly one base module exists
  int base_module_count = 0;
  for (const auto& module : config.modules) {
    if (module.is_base_module) {
      base_module_count++;
    }
    // Validate module files exist
    if (!std::filesystem::exists(module.manifest)) {
      return tl::unexpected(AndroidAabPackagerError::InvalidConfiguration);
    }
    if (!std::filesystem::exists(module.resources_apk)) {
      return tl::unexpected(AndroidAabPackagerError::InvalidConfiguration);
    }
  }

  if (base_module_count != 1) {
    return tl::unexpected(AndroidAabPackagerError::InvalidConfiguration);
  }

  // Check output path is valid
  if (config.output_aab.empty()) {
    return tl::unexpected(AndroidAabPackagerError::InvalidConfiguration);
  }

  return {};
}

auto AndroidAabPackager::compute_packaging_hash(const AabPackagingConfig& config) -> std::string {
  SHA256_CTX sha256_ctx;
  SHA256_Init(&sha256_ctx);

  // Hash all modules
  for (const auto& module : config.modules) {
    std::string module_name = module.module_name;
    SHA256_Update(&sha256_ctx, module_name.c_str(), module_name.length());

    std::string manifest_str = module.manifest.string();
    SHA256_Update(&sha256_ctx, manifest_str.c_str(), manifest_str.length());

    std::string resources_str = module.resources_apk.string();
    SHA256_Update(&sha256_ctx, resources_str.c_str(), resources_str.length());
  }

  std::array<unsigned char, SHA256_DIGEST_LENGTH> hash{};
  SHA256_Final(hash.data(), &sha256_ctx);

  std::ostringstream oss;
  for (unsigned char byte : hash) {
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }

  return oss.str();
}

auto AndroidAabPackager::find_bundletool() const -> std::optional<std::filesystem::path> {
  // Check common locations for bundletool
  std::vector<std::filesystem::path> search_paths = {
      toolchain_.sdk_root / "cmdline-tools" / "latest" / "bin" / "bundletool.jar",
      toolchain_.sdk_root / "tools" / "bin" / "bundletool.jar",
      std::filesystem::path("/usr/local/bin/bundletool.jar"),
      std::filesystem::path(std::getenv("HOME") ? std::getenv("HOME") : "") / ".android" /
          "bundletool.jar"};

  for (const auto& path : search_paths) {
    if (std::filesystem::exists(path)) {
      return path;
    }
  }

  return std::nullopt;
}

auto AndroidAabPackager::execute_bundletool(const std::vector<std::string>& args)
    -> tl::expected<std::string, AndroidAabPackagerError> {
  return execute_command(args);
}

auto AndroidAabPackager::create_module_zip(const AabModuleConfig& module,
                                           const std::filesystem::path& output_zip)
    -> tl::expected<void, AndroidAabPackagerError> {
  // Create module directory structure
  auto temp_dir = create_temp_dir();
  auto module_dir = temp_dir / module.module_name;
  std::filesystem::create_directories(module_dir);

  // Copy manifest
  auto manifest_dest = module_dir / "manifest" / "AndroidManifest.xml";
  std::filesystem::create_directories(manifest_dest.parent_path());
  std::filesystem::copy_file(module.manifest, manifest_dest);

  // Extract and copy resources
  if (std::filesystem::exists(module.resources_apk)) {
    std::vector<std::string> unzip_args;
    unzip_args.push_back("unzip");
    unzip_args.push_back("-q");
    unzip_args.push_back("-o");
    unzip_args.push_back(module.resources_apk.string());
    unzip_args.push_back("-d");
    unzip_args.push_back((module_dir / "root").string());

    auto result = execute_command(unzip_args);
    if (!result) {
      return tl::unexpected(AndroidAabPackagerError::ModuleCreationFailed);
    }
  }

  // Copy DEX files
  if (!module.dex_files.empty()) {
    auto dex_dir = module_dir / "dex";
    std::filesystem::create_directories(dex_dir);
    for (const auto& dex_file : module.dex_files) {
      auto dest = dex_dir / dex_file.filename();
      std::filesystem::copy_file(dex_file, dest);
    }
  }

  // Copy native libraries
  if (!module.native_libs.empty()) {
    auto lib_dir = module_dir / "lib";
    std::filesystem::create_directories(lib_dir);
    for (const auto& native_lib : module.native_libs) {
      auto abi_dir = native_lib.parent_path().filename();
      auto dest_abi_dir = lib_dir / abi_dir;
      std::filesystem::create_directories(dest_abi_dir);
      auto dest = dest_abi_dir / native_lib.filename();
      std::filesystem::copy_file(native_lib, dest);
    }
  }

  // Copy assets
  if (!module.assets.empty()) {
    auto assets_dir = module_dir / "assets";
    std::filesystem::create_directories(assets_dir);
    for (const auto& asset : module.assets) {
      auto dest = assets_dir / asset.filename();
      std::filesystem::copy_file(asset, dest);
    }
  }

  // Create ZIP
  std::vector<std::string> zip_args;
  zip_args.push_back("cd");
  zip_args.push_back(module_dir.string());
  zip_args.push_back("&&");
  zip_args.push_back("zip");
  zip_args.push_back("-q");
  zip_args.push_back("-r");
  zip_args.push_back(output_zip.string());
  zip_args.push_back(".");

  std::string zip_cmd;
  for (const auto& arg : zip_args) {
    zip_cmd += arg + " ";
  }

  auto result = execute_command({"/bin/sh", "-c", zip_cmd});
  if (!result) {
    return tl::unexpected(AndroidAabPackagerError::ModuleCreationFailed);
  }

  // Clean up temp directory
  std::filesystem::remove_all(temp_dir);

  return {};
}

auto AndroidAabPackager::execute_command(const std::vector<std::string>& args)
    -> tl::expected<std::string, AndroidAabPackagerError> {
  // Build command string
  std::ostringstream cmd_oss;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) {
      cmd_oss << " ";
    }
    // Quote arguments with spaces
    if (args[i].find(' ') != std::string::npos) {
      cmd_oss << "\"" << args[i] << "\"";
    } else {
      cmd_oss << args[i];
    }
  }

  cmd_oss << " 2>&1";

  std::string cmd = cmd_oss.str();

  // Execute command and capture output
  std::array<char, 128> buffer{};
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

  if (!pipe) {
    return tl::unexpected(AndroidAabPackagerError::IoError);
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  int exit_code = pclose(pipe.release());
  if (exit_code != 0) {
    return tl::unexpected(AndroidAabPackagerError::PackagingFailed);
  }

  return result;
}

auto AndroidAabPackager::create_temp_dir() const -> std::filesystem::path {
  auto temp_base = std::filesystem::temp_directory_path() / "horcrux_aab";
  std::filesystem::create_directories(temp_base);

  auto temp_dir = temp_base / std::to_string(std::time(nullptr));
  std::filesystem::create_directories(temp_dir);

  return temp_dir;
}

// Utility functions
namespace aab_utils {

auto get_aab_info(const std::filesystem::path& aab_path)
    -> tl::expected<AabInfo, AndroidAabPackagerError> {
  // This would use bundletool to get AAB info
  // For now, return a placeholder
  AabInfo info;
  info.module_names.push_back("base");
  info.package_name = "com.example.app";
  info.version_name = "1.0";
  info.version_code = 1;
  info.min_sdk_version = 21;
  info.target_sdk_version = 34;
  return info;
}

auto validate_aab(const std::filesystem::path& aab_path)
    -> tl::expected<bool, AndroidAabPackagerError> {
  if (!std::filesystem::exists(aab_path)) {
    return tl::unexpected(AndroidAabPackagerError::InvalidConfiguration);
  }
  return true;
}

auto extract_aab(const std::filesystem::path& aab_path, const std::filesystem::path& output_dir)
    -> tl::expected<void, AndroidAabPackagerError> {
  std::filesystem::create_directories(output_dir);

  std::vector<std::string> args;
  args.push_back("unzip");
  args.push_back("-q");
  args.push_back("-o");
  args.push_back(aab_path.string());
  args.push_back("-d");
  args.push_back(output_dir.string());

  std::string cmd;
  for (const auto& arg : args) {
    cmd += arg + " ";
  }

  int result = std::system(cmd.c_str());
  if (result != 0) {
    return tl::unexpected(AndroidAabPackagerError::IoError);
  }

  return {};
}

} // namespace aab_utils

} // namespace horcrux::core
