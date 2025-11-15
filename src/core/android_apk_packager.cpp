// Horcrux - Android APK Packaging Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_apk_packager.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>

#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace horcrux::core {

// Convert error to string
auto to_string(AndroidApkPackagerError error) -> std::string {
  switch (error) {
    case AndroidApkPackagerError::InvalidConfiguration:
      return "Invalid configuration";
    case AndroidApkPackagerError::ZipalignNotFound:
      return "zipalign tool not found";
    case AndroidApkPackagerError::ApksignerNotFound:
      return "apksigner tool not found";
    case AndroidApkPackagerError::SigningFailed:
      return "APK signing failed";
    case AndroidApkPackagerError::VerificationFailed:
      return "APK verification failed";
    case AndroidApkPackagerError::PackagingFailed:
      return "APK packaging failed";
    case AndroidApkPackagerError::IoError:
      return "I/O error";
    case AndroidApkPackagerError::UnknownError:
      return "Unknown error";
  }
  return "Unknown error";
}

// AndroidApkPackager implementation
AndroidApkPackager::AndroidApkPackager(const AndroidToolchain& toolchain)
    : toolchain_(toolchain), zipalign_path_(find_zipalign()),
      apksigner_path_(find_apksigner()) {}

auto AndroidApkPackager::package(const ApkPackagingConfig& config)
    -> tl::expected<ApkPackagingResult, AndroidApkPackagerError> {
  auto start_time = std::chrono::steady_clock::now();

  // Validate configuration
  if (auto validation = validate_config(config); !validation) {
    return tl::unexpected(validation.error());
  }

  // Create temporary working directory
  auto temp_dir = create_temp_dir();

  // Package APK internal
  auto packaged_apk_result = package_apk_internal(config, temp_dir);
  if (!packaged_apk_result) {
    return tl::unexpected(packaged_apk_result.error());
  }

  auto current_apk = *packaged_apk_result;

  // Zipalign if requested
  if (config.zipalign) {
    auto zipaligned_apk = temp_dir / "aligned.apk";
    if (auto result = zipalign(current_apk, zipaligned_apk, config.zipalign_alignment);
        !result) {
      return tl::unexpected(result.error());
    }
    current_apk = zipaligned_apk;
  }

  // Sign if signing config provided
  bool is_signed = false;
  if (config.signing_config) {
    auto signed_apk = temp_dir / "signed.apk";
    if (auto result = sign(current_apk, signed_apk, *config.signing_config); !result) {
      return tl::unexpected(result.error());
    }
    current_apk = signed_apk;
    is_signed = true;
  }

  // Verify signature if requested
  bool is_verified = false;
  if (is_signed && config.verify_signature) {
    if (auto result = verify(current_apk); result && result->is_valid) {
      is_verified = true;
    } else {
      return tl::unexpected(AndroidApkPackagerError::VerificationFailed);
    }
  }

  // Copy final APK to output location
  std::filesystem::create_directories(config.output_apk.parent_path());
  std::filesystem::copy_file(current_apk, config.output_apk,
                            std::filesystem::copy_options::overwrite_existing);

  // Clean up temporary directory
  std::filesystem::remove_all(temp_dir);

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  ApkPackagingResult result;
  result.output_apk = config.output_apk;
  result.is_signed = is_signed;
  result.is_verified = is_verified;
  result.packaging_time = duration;
  result.packaging_hash = compute_packaging_hash(config);

  return result;
}

auto AndroidApkPackager::zipalign(const std::filesystem::path& input_apk,
                                   const std::filesystem::path& output_apk,
                                   int alignment)
    -> tl::expected<void, AndroidApkPackagerError> {
  if (!zipalign_path_ || !std::filesystem::exists(*zipalign_path_)) {
    return tl::unexpected(AndroidApkPackagerError::ZipalignNotFound);
  }

  std::vector<std::string> args;
  args.push_back(zipalign_path_->string());
  args.push_back("-f"); // Force overwrite
  args.push_back(std::to_string(alignment));
  args.push_back(input_apk.string());
  args.push_back(output_apk.string());

  auto result = execute_command(args);
  if (!result) {
    return tl::unexpected(result.error());
  }

  return {};
}

auto AndroidApkPackager::sign(const std::filesystem::path& input_apk,
                               const std::filesystem::path& output_apk,
                               const ApkSigningConfig& signing_config)
    -> tl::expected<void, AndroidApkPackagerError> {
  if (!apksigner_path_ || !std::filesystem::exists(*apksigner_path_)) {
    return tl::unexpected(AndroidApkPackagerError::ApksignerNotFound);
  }

  // Check if keystore exists
  if (!std::filesystem::exists(signing_config.keystore_path)) {
    return tl::unexpected(AndroidApkPackagerError::InvalidConfiguration);
  }

  std::vector<std::string> args;
  args.push_back(apksigner_path_->string());
  args.push_back("sign");
  args.push_back("--ks");
  args.push_back(signing_config.keystore_path.string());
  args.push_back("--ks-pass");
  args.push_back("pass:" + signing_config.keystore_password);
  args.push_back("--key-pass");
  args.push_back("pass:" + signing_config.key_password);
  args.push_back("--ks-key-alias");
  args.push_back(signing_config.key_alias);
  args.push_back("--out");
  args.push_back(output_apk.string());
  args.push_back(input_apk.string());

  auto result = execute_command(args);
  if (!result) {
    return tl::unexpected(AndroidApkPackagerError::SigningFailed);
  }

  return {};
}

auto AndroidApkPackager::verify(const std::filesystem::path& apk_path)
    -> tl::expected<ApkVerificationResult, AndroidApkPackagerError> {
  if (!apksigner_path_ || !std::filesystem::exists(*apksigner_path_)) {
    return tl::unexpected(AndroidApkPackagerError::ApksignerNotFound);
  }

  std::vector<std::string> args;
  args.push_back(apksigner_path_->string());
  args.push_back("verify");
  args.push_back("--verbose");
  args.push_back(apk_path.string());

  auto result = execute_command(args);

  ApkVerificationResult verification_result;
  if (result) {
    verification_result.is_valid = true;
    // Parse signatures from output
    // Example output: "Verified using v1 scheme (JAR signing): true"
    if (result->find("v1 scheme") != std::string::npos) {
      verification_result.signatures.push_back("v1");
    }
    if (result->find("v2 scheme") != std::string::npos) {
      verification_result.signatures.push_back("v2");
    }
    if (result->find("v3 scheme") != std::string::npos) {
      verification_result.signatures.push_back("v3");
    }
    if (result->find("v4 scheme") != std::string::npos) {
      verification_result.signatures.push_back("v4");
    }
  } else {
    verification_result.is_valid = false;
    verification_result.error_message = to_string(result.error());
  }

  return verification_result;
}

auto AndroidApkPackager::get_zipalign_path() const
    -> std::optional<std::filesystem::path> {
  return zipalign_path_;
}

auto AndroidApkPackager::get_apksigner_path() const
    -> std::optional<std::filesystem::path> {
  return apksigner_path_;
}

auto AndroidApkPackager::validate_config(const ApkPackagingConfig& config)
    -> tl::expected<void, AndroidApkPackagerError> {
  // Check resources APK exists
  if (!std::filesystem::exists(config.resources_apk)) {
    return tl::unexpected(AndroidApkPackagerError::InvalidConfiguration);
  }

  // Check at least one DEX file exists
  if (config.dex_files.empty()) {
    return tl::unexpected(AndroidApkPackagerError::InvalidConfiguration);
  }

  for (const auto& dex_file : config.dex_files) {
    if (!std::filesystem::exists(dex_file)) {
      return tl::unexpected(AndroidApkPackagerError::InvalidConfiguration);
    }
  }

  // Check output path is valid
  if (config.output_apk.empty()) {
    return tl::unexpected(AndroidApkPackagerError::InvalidConfiguration);
  }

  // Check signing config if provided
  if (config.signing_config) {
    if (!std::filesystem::exists(config.signing_config->keystore_path)) {
      return tl::unexpected(AndroidApkPackagerError::InvalidConfiguration);
    }
  }

  return {};
}

auto AndroidApkPackager::compute_packaging_hash(const ApkPackagingConfig& config)
    -> std::string {
  SHA256_CTX sha256_ctx;
  SHA256_Init(&sha256_ctx);

  // Hash resources APK path
  std::string resources_apk_str = config.resources_apk.string();
  SHA256_Update(&sha256_ctx, resources_apk_str.c_str(), resources_apk_str.length());

  // Hash DEX files
  for (const auto& dex_file : config.dex_files) {
    std::string dex_str = dex_file.string();
    SHA256_Update(&sha256_ctx, dex_str.c_str(), dex_str.length());
  }

  // Hash native libs
  for (const auto& native_lib : config.native_libs) {
    std::string lib_str = native_lib.string();
    SHA256_Update(&sha256_ctx, lib_str.c_str(), lib_str.length());
  }

  // Hash assets
  for (const auto& asset : config.assets) {
    std::string asset_str = asset.string();
    SHA256_Update(&sha256_ctx, asset_str.c_str(), asset_str.length());
  }

  std::array<unsigned char, SHA256_DIGEST_LENGTH> hash{};
  SHA256_Final(hash.data(), &sha256_ctx);

  std::ostringstream oss;
  for (unsigned char byte : hash) {
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }

  return oss.str();
}

auto AndroidApkPackager::find_zipalign() const
    -> std::optional<std::filesystem::path> {
  // Find the latest build tools with zipalign
  for (const auto& bt : toolchain_.build_tools) {
    if (std::filesystem::exists(bt.zipalign_path)) {
      return bt.zipalign_path;
    }
  }
  return std::nullopt;
}

auto AndroidApkPackager::find_apksigner() const
    -> std::optional<std::filesystem::path> {
  // Find apksigner in the latest build tools
  for (const auto& bt : toolchain_.build_tools) {
    // apksigner is a script (not .exe on Windows)
    auto apksigner_path = bt.path / "apksigner";
    if (std::filesystem::exists(apksigner_path)) {
      return apksigner_path;
    }
    // Try with .bat extension on Windows
    auto apksigner_bat = bt.path / "apksigner.bat";
    if (std::filesystem::exists(apksigner_bat)) {
      return apksigner_bat;
    }
  }
  return std::nullopt;
}

auto AndroidApkPackager::execute_command(
    const std::vector<std::string>& args,
    const std::optional<std::filesystem::path>& working_dir)
    -> tl::expected<std::string, AndroidApkPackagerError> {
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

  // Redirect stderr to stdout
  cmd_oss << " 2>&1";

  std::string cmd = cmd_oss.str();

  // Execute command and capture output
  std::array<char, 128> buffer{};
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

  if (!pipe) {
    return tl::unexpected(AndroidApkPackagerError::IoError);
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  int exit_code = pclose(pipe.release());
  if (exit_code != 0) {
    return tl::unexpected(AndroidApkPackagerError::PackagingFailed);
  }

  return result;
}

auto AndroidApkPackager::create_temp_dir() const -> std::filesystem::path {
  auto temp_base = std::filesystem::temp_directory_path() / "horcrux_apk";
  std::filesystem::create_directories(temp_base);
  
  // Create unique temp directory
  auto temp_dir = temp_base / std::to_string(std::time(nullptr));
  std::filesystem::create_directories(temp_dir);
  
  return temp_dir;
}

auto AndroidApkPackager::package_apk_internal(
    const ApkPackagingConfig& config,
    const std::filesystem::path& temp_dir)
    -> tl::expected<std::filesystem::path, AndroidApkPackagerError> {
  // Create APK structure in temp directory
  auto apk_contents = temp_dir / "apk_contents";
  std::filesystem::create_directories(apk_contents);

  // Extract resources APK (which is a ZIP file)
  std::vector<std::string> unzip_args;
  unzip_args.push_back("unzip");
  unzip_args.push_back("-q"); // Quiet
  unzip_args.push_back("-o"); // Overwrite
  unzip_args.push_back(config.resources_apk.string());
  unzip_args.push_back("-d");
  unzip_args.push_back(apk_contents.string());

  auto unzip_result = execute_command(unzip_args);
  if (!unzip_result) {
    return tl::unexpected(unzip_result.error());
  }

  // Copy DEX files
  for (const auto& dex_file : config.dex_files) {
    auto dest = apk_contents / dex_file.filename();
    std::filesystem::copy_file(dex_file, dest,
                              std::filesystem::copy_options::overwrite_existing);
  }

  // Copy native libraries
  if (!config.native_libs.empty()) {
    auto lib_dir = apk_contents / "lib";
    std::filesystem::create_directories(lib_dir);
    for (const auto& native_lib : config.native_libs) {
      // Detect ABI from path (e.g., lib/armeabi-v7a/libfoo.so)
      auto abi_dir = native_lib.parent_path().filename();
      auto dest_abi_dir = lib_dir / abi_dir;
      std::filesystem::create_directories(dest_abi_dir);
      auto dest = dest_abi_dir / native_lib.filename();
      std::filesystem::copy_file(native_lib, dest,
                                std::filesystem::copy_options::overwrite_existing);
    }
  }

  // Copy assets
  if (!config.assets.empty()) {
    auto assets_dir = apk_contents / "assets";
    std::filesystem::create_directories(assets_dir);
    for (const auto& asset : config.assets) {
      auto dest = assets_dir / asset.filename();
      std::filesystem::copy_file(asset, dest,
                                std::filesystem::copy_options::overwrite_existing);
    }
  }

  // Create APK (ZIP file)
  auto output_apk = temp_dir / "unaligned.apk";
  std::vector<std::string> zip_args;
  zip_args.push_back("cd");
  zip_args.push_back(apk_contents.string());
  zip_args.push_back("&&");
  zip_args.push_back("zip");
  zip_args.push_back("-q"); // Quiet
  zip_args.push_back("-r"); // Recursive
  zip_args.push_back(output_apk.string());
  zip_args.push_back(".");

  // Execute with shell
  std::string zip_cmd;
  for (const auto& arg : zip_args) {
    zip_cmd += arg + " ";
  }

  auto zip_result = execute_command({"/bin/sh", "-c", zip_cmd});
  if (!zip_result) {
    return tl::unexpected(zip_result.error());
  }

  return output_apk;
}

// Utility functions
namespace apk_utils {

auto is_aligned(const std::filesystem::path& apk_path, int alignment) -> bool {
  // Simple check: file size should be aligned
  auto file_size = std::filesystem::file_size(apk_path);
  return (file_size % alignment) == 0;
}

auto extract_apk(const std::filesystem::path& apk_path,
                 const std::filesystem::path& output_dir)
    -> tl::expected<void, AndroidApkPackagerError> {
  std::filesystem::create_directories(output_dir);

  std::vector<std::string> args;
  args.push_back("unzip");
  args.push_back("-q");
  args.push_back("-o");
  args.push_back(apk_path.string());
  args.push_back("-d");
  args.push_back(output_dir.string());

  // Execute command
  std::string cmd;
  for (const auto& arg : args) {
    cmd += arg + " ";
  }

  int result = std::system(cmd.c_str());
  if (result != 0) {
    return tl::unexpected(AndroidApkPackagerError::IoError);
  }

  return {};
}

auto get_apk_info(const std::filesystem::path& apk_path)
    -> tl::expected<ApkInfo, AndroidApkPackagerError> {
  // This would use aapt dump badging to get APK info
  // For now, return a placeholder
  ApkInfo info;
  info.package_name = "com.example.app";
  info.version_name = "1.0";
  info.version_code = 1;
  info.min_sdk_version = 21;
  info.target_sdk_version = 34;
  return info;
}

} // namespace apk_utils

} // namespace horcrux::core
