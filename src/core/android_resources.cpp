// Horcrux - Android Resource Processing (AAPT2 Pipeline)
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_resources.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <memory>
#include <regex>
#include <sstream>

#include <openssl/sha.h>

namespace horcrux::core {

// Error to string conversion
auto to_string(AndroidResourceError error) -> std::string {
  switch (error) {
  case AndroidResourceError::Aapt2NotFound:
    return "AAPT2 not found in Android build tools";
  case AndroidResourceError::InvalidResourceFile:
    return "Invalid resource file";
  case AndroidResourceError::InvalidManifest:
    return "Invalid AndroidManifest.xml";
  case AndroidResourceError::CompilationFailed:
    return "Resource compilation failed";
  case AndroidResourceError::LinkingFailed:
    return "Resource linking failed";
  case AndroidResourceError::MergingFailed:
    return "Resource merging failed";
  case AndroidResourceError::InvalidConfiguration:
    return "Invalid configuration";
  case AndroidResourceError::IoError:
    return "I/O error";
  case AndroidResourceError::UnknownError:
    return "Unknown error";
  }
  return "Unknown error";
}

// ResourceQualifiers implementation
auto ResourceQualifiers::parse(const std::string& dir_name) -> ResourceQualifiers {
  ResourceQualifiers qualifiers;

  // Split by '-'
  std::vector<std::string> parts;
  std::stringstream ss(dir_name);
  std::string part;
  while (std::getline(ss, part, '-')) {
    parts.push_back(part);
  }

  // First part is resource type (values, layout, etc.), skip it
  for (size_t i = 1; i < parts.size(); ++i) {
    const auto& p = parts[i];

    // Check for locale (2 letters)
    if (p.length() == 2 && std::isalpha(p[0]) && std::isalpha(p[1])) {
      qualifiers.locale = p;
    }
    // Check for region (r + 2 letters)
    else if (p.length() == 3 && p[0] == 'r' && std::isupper(p[1]) && std::isupper(p[2])) {
      qualifiers.region = p.substr(1);
    }
    // Check for API level (v + number)
    else if (p[0] == 'v' && p.length() > 1 && std::isdigit(p[1])) {
      try {
        qualifiers.api_level = std::stoi(p.substr(1));
      } catch (...) {
      }
    }
    // Check for density
    else if (p == "ldpi") {
      qualifiers.density = ResourceDensity::LDPI;
    } else if (p == "mdpi") {
      qualifiers.density = ResourceDensity::MDPI;
    } else if (p == "hdpi") {
      qualifiers.density = ResourceDensity::HDPI;
    } else if (p == "xhdpi") {
      qualifiers.density = ResourceDensity::XHDPI;
    } else if (p == "xxhdpi") {
      qualifiers.density = ResourceDensity::XXHDPI;
    } else if (p == "xxxhdpi") {
      qualifiers.density = ResourceDensity::XXXHDPI;
    } else if (p == "nodpi") {
      qualifiers.density = ResourceDensity::NODPI;
    } else if (p == "tvdpi") {
      qualifiers.density = ResourceDensity::TVDPI;
    } else if (p == "anydpi") {
      qualifiers.density = ResourceDensity::ANYDPI;
    }
    // Check for screen size
    else if (p == "small" || p == "normal" || p == "large" || p == "xlarge") {
      qualifiers.screen_size = p;
    }
    // Check for orientation
    else if (p == "port" || p == "land") {
      qualifiers.orientation = p;
    }
    // Check for night mode
    else if (p == "night" || p == "notnight") {
      qualifiers.night_mode = p;
    }
  }

  return qualifiers;
}

auto ResourceQualifiers::to_string() const -> std::string {
  std::stringstream ss;

  if (locale)
    ss << *locale << "-";
  if (region)
    ss << "r" << *region << "-";
  if (density)
    ss << resource_utils::density_to_string(*density) << "-";
  if (screen_size)
    ss << *screen_size << "-";
  if (orientation)
    ss << *orientation << "-";
  if (night_mode)
    ss << *night_mode << "-";
  if (api_level)
    ss << "v" << *api_level;

  std::string result = ss.str();
  if (!result.empty() && result.back() == '-') {
    result.pop_back();
  }
  return result;
}

// ResourceFile implementation
auto ResourceFile::get_type(const std::filesystem::path& path) -> ResourceType {
  if (!path.has_parent_path())
    return ResourceType::Unknown;

  auto parent = path.parent_path().filename().string();

  // Extract base type (before qualifiers)
  auto pos = parent.find('-');
  std::string base_type = (pos != std::string::npos) ? parent.substr(0, pos) : parent;

  if (base_type == "values")
    return ResourceType::Values;
  if (base_type == "layout")
    return ResourceType::Layout;
  if (base_type == "drawable")
    return ResourceType::Drawable;
  if (base_type == "mipmap")
    return ResourceType::Mipmap;
  if (base_type == "raw")
    return ResourceType::Raw;
  if (base_type == "xml")
    return ResourceType::Xml;
  if (base_type == "anim")
    return ResourceType::Anim;
  if (base_type == "animator")
    return ResourceType::Animator;
  if (base_type == "color")
    return ResourceType::Color;
  if (base_type == "menu")
    return ResourceType::Menu;

  return ResourceType::Unknown;
}

auto ResourceFile::compute_hash(const std::filesystem::path& path)
    -> tl::expected<std::string, AndroidResourceError> {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return tl::unexpected(AndroidResourceError::IoError);
  }

  SHA256_CTX sha256_ctx;
  SHA256_Init(&sha256_ctx);

  std::array<char, 8192> buffer;
  while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
    SHA256_Update(&sha256_ctx, buffer.data(), file.gcount());
  }

  std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;
  SHA256_Final(hash.data(), &sha256_ctx);

  std::stringstream ss;
  for (const auto& byte : hash) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }

  return ss.str();
}

// AndroidManifest implementation
auto AndroidManifest::parse(const std::filesystem::path& manifest_path)
    -> tl::expected<AndroidManifest, AndroidResourceError> {
  if (!std::filesystem::exists(manifest_path)) {
    return tl::unexpected(AndroidResourceError::InvalidManifest);
  }

  AndroidManifest manifest;
  manifest.path = manifest_path;

  // Simple XML parsing - in production, use proper XML library
  std::ifstream file(manifest_path);
  if (!file) {
    return tl::unexpected(AndroidResourceError::IoError);
  }

  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  // Extract package name
  std::regex package_regex(R"regex(package\s*=\s*"([^"]+)")regex");
  std::smatch match;
  if (std::regex_search(content, match, package_regex)) {
    manifest.package_name = match[1].str();
  }

  // Extract version name
  std::regex version_name_regex(R"regex(android:versionName\s*=\s*"([^"]+)")regex");
  if (std::regex_search(content, match, version_name_regex)) {
    manifest.version_name = match[1].str();
  }

  // Extract version code
  std::regex version_code_regex(R"regex(android:versionCode\s*=\s*"(\d+)")regex");
  if (std::regex_search(content, match, version_code_regex)) {
    manifest.version_code = std::stoi(match[1].str());
  }

  // Extract minSdkVersion
  std::regex min_sdk_regex(R"regex(android:minSdkVersion\s*=\s*"(\d+)")regex");
  if (std::regex_search(content, match, min_sdk_regex)) {
    manifest.min_sdk_version = std::stoi(match[1].str());
  }

  // Extract targetSdkVersion
  std::regex target_sdk_regex(R"regex(android:targetSdkVersion\s*=\s*"(\d+)")regex");
  if (std::regex_search(content, match, target_sdk_regex)) {
    manifest.target_sdk_version = std::stoi(match[1].str());
  }

  // Extract maxSdkVersion
  std::regex max_sdk_regex(R"regex(android:maxSdkVersion\s*=\s*"(\d+)")regex");
  if (std::regex_search(content, match, max_sdk_regex)) {
    manifest.max_sdk_version = std::stoi(match[1].str());
  }

  // Compute content hash
  auto hash_result = ResourceFile::compute_hash(manifest_path);
  if (!hash_result) {
    return tl::unexpected(hash_result.error());
  }
  manifest.content_hash = *hash_result;

  return manifest;
}

// AndroidResourceProcessor implementation
AndroidResourceProcessor::AndroidResourceProcessor(const AndroidToolchain& toolchain)
    : toolchain_(toolchain) {
  aapt2_path_ = find_aapt2();
}

auto AndroidResourceProcessor::get_aapt2_path() const -> std::optional<std::filesystem::path> {
  return aapt2_path_;
}

auto AndroidResourceProcessor::find_aapt2() const -> std::optional<std::filesystem::path> {
  // Search for AAPT2 in build tools (prefer latest version)
  for (auto it = toolchain_.build_tools.rbegin(); it != toolchain_.build_tools.rend(); ++it) {
#ifdef _WIN32
    auto aapt2_path = it->path / "aapt2.exe";
#else
    auto aapt2_path = it->path / "aapt2";
#endif

    if (std::filesystem::exists(aapt2_path)) {
      return aapt2_path;
    }
  }

  return std::nullopt;
}

auto AndroidResourceProcessor::execute_aapt2(const std::vector<std::string>& args)
    -> tl::expected<std::string, AndroidResourceError> {
  if (!aapt2_path_) {
    return tl::unexpected(AndroidResourceError::Aapt2NotFound);
  }

  // Build command
  std::string command = aapt2_path_->string();
  for (const auto& arg : args) {
    command += " \"" + arg + "\"";
  }

  // Execute command and capture output
  std::array<char, 128> buffer;
  std::string result;

  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

  if (!pipe) {
    return tl::unexpected(AndroidResourceError::IoError);
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  // Check exit code
  int exit_code = pclose(pipe.release());
  if (exit_code != 0) {
    return tl::unexpected(AndroidResourceError::CompilationFailed);
  }

  return result;
}

auto AndroidResourceProcessor::compile(const Aapt2CompileConfig& config)
    -> tl::expected<Aapt2CompileResult, AndroidResourceError> {
  // Validate configuration
  auto validation = validate_compile_config(config);
  if (!validation) {
    return tl::unexpected(validation.error());
  }

  if (!aapt2_path_) {
    return tl::unexpected(AndroidResourceError::Aapt2NotFound);
  }

  // Create output directory
  std::filesystem::create_directories(config.output_dir);

  auto start_time = std::chrono::steady_clock::now();

  Aapt2CompileResult result;

  // Compile each resource file
  for (const auto& resource : config.resources) {
    std::vector<std::string> args;
    args.push_back("compile");

    if (config.verbose) {
      args.push_back("-v");
    }

    args.push_back("-o");
    args.push_back(config.output_dir.string());

    // Add additional arguments
    for (const auto& arg : config.additional_args) {
      args.push_back(arg);
    }

    args.push_back(resource.path.string());

    // Execute AAPT2 compile
    auto exec_result = execute_aapt2(args);
    if (!exec_result) {
      return tl::unexpected(exec_result.error());
    }

    // Determine output .flat file name
    auto flat_name = resource.path.filename().string() + ".flat";
    auto flat_path = config.output_dir / flat_name;

    if (std::filesystem::exists(flat_path)) {
      result.compiled_files.push_back(flat_path);
    }
  }

  auto end_time = std::chrono::steady_clock::now();
  result.compilation_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Compute compilation hash
  result.compilation_hash = compute_compile_hash(config);

  return result;
}

auto AndroidResourceProcessor::link(const Aapt2LinkConfig& config)
    -> tl::expected<Aapt2LinkResult, AndroidResourceError> {
  // Validate configuration
  auto validation = validate_link_config(config);
  if (!validation) {
    return tl::unexpected(validation.error());
  }

  if (!aapt2_path_) {
    return tl::unexpected(AndroidResourceError::Aapt2NotFound);
  }

  auto start_time = std::chrono::steady_clock::now();

  std::vector<std::string> args;
  args.push_back("link");

  if (config.verbose) {
    args.push_back("-v");
  }

  // Add manifest
  args.push_back("--manifest");
  args.push_back(config.manifest.string());

  // Add android.jar
  args.push_back("-I");
  args.push_back(config.android_jar.string());

  // Add output APK
  args.push_back("-o");
  args.push_back(config.output_apk.string());

  // Add R.java output directory
  if (config.r_java_output) {
    args.push_back("--java");
    args.push_back(config.r_java_output->string());
  }

  // Add ProGuard output
  if (config.proguard_output) {
    args.push_back("--proguard");
    args.push_back(config.proguard_output->string());
  }

  // Add package name override
  if (config.package_name) {
    args.push_back("--rename-manifest-package");
    args.push_back(*config.package_name);
  }

  // Add auto-add-overlay flag
  if (config.auto_add_overlay) {
    args.push_back("--auto-add-overlay");
  }

  // Add overlays
  for (const auto& overlay : config.overlays) {
    args.push_back("-R");
    args.push_back(overlay.string());
  }

  // Add additional arguments
  for (const auto& arg : config.additional_args) {
    args.push_back(arg);
  }

  // Add compiled resources (.flat files)
  for (const auto& compiled : config.compiled_resources) {
    args.push_back(compiled.string());
  }

  // Execute AAPT2 link
  auto exec_result = execute_aapt2(args);
  if (!exec_result) {
    return tl::unexpected(AndroidResourceError::LinkingFailed);
  }

  auto end_time = std::chrono::steady_clock::now();

  Aapt2LinkResult result;
  result.output_apk = config.output_apk;

  // Check for R.jar (generated in r_java_output)
  if (config.r_java_output) {
    // After linking, compile R.java to R.jar (simplified for now)
    // In production, this would invoke javac and jar
    result.r_jar = *config.r_java_output / "R.jar";
  }

  // Check for ProGuard rules
  if (config.proguard_output && std::filesystem::exists(*config.proguard_output)) {
    result.proguard = *config.proguard_output;
  }

  result.link_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  result.link_hash = compute_link_hash(config);

  return result;
}

auto AndroidResourceProcessor::merge_resources(const ResourceMergeConfig& config)
    -> tl::expected<ResourceMergeResult, AndroidResourceError> {
  auto start_time = std::chrono::steady_clock::now();

  // Create output directory
  std::filesystem::create_directories(config.output_dir);

  ResourceMergeResult result;
  result.merged_dir = config.output_dir;

  // Collect all resources from all directories
  std::vector<ResourceFile> all_resources;

  for (const auto& res_dir : config.resource_dirs) {
    auto scan_result = resource_utils::scan_resources(res_dir);
    if (!scan_result) {
      return tl::unexpected(scan_result.error());
    }

    all_resources.insert(all_resources.end(), scan_result->begin(), scan_result->end());
  }

  // Sort resources if deterministic merge is requested
  if (config.deterministic) {
    resource_utils::sort_resources(all_resources);
  }

  // Merge resources (copy to output directory)
  for (const auto& resource : all_resources) {
    auto relative_path =
        std::filesystem::relative(resource.path, resource.path.parent_path().parent_path());
    auto dest_path = config.output_dir / relative_path;

    std::filesystem::create_directories(dest_path.parent_path());
    std::filesystem::copy_file(resource.path, dest_path,
                               std::filesystem::copy_options::overwrite_existing);

    result.merged_resources.push_back(resource);
  }

  auto end_time = std::chrono::steady_clock::now();
  result.merge_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Compute merge hash
  SHA256_CTX sha256_ctx;
  SHA256_Init(&sha256_ctx);

  for (const auto& resource : result.merged_resources) {
    SHA256_Update(&sha256_ctx, resource.content_hash.c_str(), resource.content_hash.length());
  }

  std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;
  SHA256_Final(hash.data(), &sha256_ctx);

  std::stringstream ss;
  for (const auto& byte : hash) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }
  result.merge_hash = ss.str();

  return result;
}

auto AndroidResourceProcessor::merge_manifests(const ManifestMergeConfig& config)
    -> tl::expected<ManifestMergeResult, AndroidResourceError> {
  auto start_time = std::chrono::steady_clock::now();

  // Simple manifest merge: copy main manifest
  // In production, use proper manifest merger tool
  std::filesystem::create_directories(config.output_manifest.parent_path());
  std::filesystem::copy_file(config.main_manifest, config.output_manifest,
                             std::filesystem::copy_options::overwrite_existing);

  auto end_time = std::chrono::steady_clock::now();

  ManifestMergeResult result;
  result.merged_manifest = config.output_manifest;
  result.merge_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Compute merge hash
  auto hash_result = ResourceFile::compute_hash(config.output_manifest);
  if (!hash_result) {
    return tl::unexpected(hash_result.error());
  }
  result.merge_hash = *hash_result;

  return result;
}

auto AndroidResourceProcessor::process_pipeline(const PipelineConfig& config)
    -> tl::expected<PipelineResult, AndroidResourceError> {
  auto start_time = std::chrono::steady_clock::now();

  // Step 1: Merge resources
  ResourceMergeConfig merge_config;
  merge_config.resource_dirs = config.resource_dirs;
  merge_config.output_dir = config.output_dir / "merged_res";
  merge_config.deterministic = true;
  merge_config.verbose = config.verbose;

  auto merge_result = merge_resources(merge_config);
  if (!merge_result) {
    return tl::unexpected(merge_result.error());
  }

  // Step 2: Compile resources
  Aapt2CompileConfig compile_config;
  compile_config.resources = merge_result->merged_resources;
  compile_config.output_dir = config.output_dir / "compiled";
  compile_config.incremental = config.incremental;
  compile_config.verbose = config.verbose;

  auto compile_result = compile(compile_config);
  if (!compile_result) {
    return tl::unexpected(compile_result.error());
  }

  // Step 3: Link resources
  Aapt2LinkConfig link_config;
  link_config.compiled_resources = compile_result->compiled_files;
  link_config.manifest = config.manifest;
  link_config.output_apk = config.output_dir / "resources.ap_";
  link_config.r_java_output = config.output_dir / "gen";
  link_config.android_jar = config.android_jar;
  link_config.package_name = config.package_name;
  link_config.verbose = config.verbose;

  auto link_result = link(link_config);
  if (!link_result) {
    return tl::unexpected(link_result.error());
  }

  auto end_time = std::chrono::steady_clock::now();

  PipelineResult result;
  result.resources_apk = link_result->output_apk;
  result.r_jar = link_result->r_jar.value_or(config.output_dir / "R.jar");
  result.processed_manifest = config.manifest;
  result.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Compute pipeline hash
  SHA256_CTX sha256_ctx;
  SHA256_Init(&sha256_ctx);

  SHA256_Update(&sha256_ctx, merge_result->merge_hash.c_str(), merge_result->merge_hash.length());
  SHA256_Update(&sha256_ctx, compile_result->compilation_hash.c_str(),
                compile_result->compilation_hash.length());
  SHA256_Update(&sha256_ctx, link_result->link_hash.c_str(), link_result->link_hash.length());

  std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;
  SHA256_Final(hash.data(), &sha256_ctx);

  std::stringstream ss;
  for (const auto& byte : hash) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }
  result.pipeline_hash = ss.str();

  return result;
}

auto AndroidResourceProcessor::validate_compile_config(const Aapt2CompileConfig& config)
    -> tl::expected<void, AndroidResourceError> {
  if (config.resources.empty()) {
    return tl::unexpected(AndroidResourceError::InvalidConfiguration);
  }

  for (const auto& resource : config.resources) {
    if (!std::filesystem::exists(resource.path)) {
      return tl::unexpected(AndroidResourceError::InvalidResourceFile);
    }
  }

  return {};
}

auto AndroidResourceProcessor::validate_link_config(const Aapt2LinkConfig& config)
    -> tl::expected<void, AndroidResourceError> {
  if (config.compiled_resources.empty()) {
    return tl::unexpected(AndroidResourceError::InvalidConfiguration);
  }

  if (!std::filesystem::exists(config.manifest)) {
    return tl::unexpected(AndroidResourceError::InvalidManifest);
  }

  if (!std::filesystem::exists(config.android_jar)) {
    return tl::unexpected(AndroidResourceError::InvalidConfiguration);
  }

  return {};
}

auto AndroidResourceProcessor::compute_compile_hash(const Aapt2CompileConfig& config)
    -> std::string {
  SHA256_CTX sha256_ctx;
  SHA256_Init(&sha256_ctx);

  // Hash all resource files (sorted for determinism)
  std::vector<std::string> hashes;
  for (const auto& resource : config.resources) {
    hashes.push_back(resource.content_hash);
  }
  std::sort(hashes.begin(), hashes.end());

  for (const auto& hash : hashes) {
    SHA256_Update(&sha256_ctx, hash.c_str(), hash.length());
  }

  // Hash additional arguments (sorted)
  auto sorted_args = config.additional_args;
  std::sort(sorted_args.begin(), sorted_args.end());
  for (const auto& arg : sorted_args) {
    SHA256_Update(&sha256_ctx, arg.c_str(), arg.length());
  }

  std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;
  SHA256_Final(hash.data(), &sha256_ctx);

  std::stringstream ss;
  for (const auto& byte : hash) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }

  return ss.str();
}

auto AndroidResourceProcessor::compute_link_hash(const Aapt2LinkConfig& config) -> std::string {
  SHA256_CTX sha256_ctx;
  SHA256_Init(&sha256_ctx);

  // Hash manifest
  auto manifest_hash_result = ResourceFile::compute_hash(config.manifest);
  if (manifest_hash_result) {
    SHA256_Update(&sha256_ctx, manifest_hash_result->c_str(), manifest_hash_result->length());
  }

  // Hash compiled resources (sorted for determinism)
  std::vector<std::string> paths;
  for (const auto& compiled : config.compiled_resources) {
    paths.push_back(compiled.string());
  }
  std::sort(paths.begin(), paths.end());

  for (const auto& path : paths) {
    SHA256_Update(&sha256_ctx, path.c_str(), path.length());
  }

  // Hash android.jar path
  std::string android_jar_str = config.android_jar.string();
  SHA256_Update(&sha256_ctx, android_jar_str.c_str(), android_jar_str.length());

  std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;
  SHA256_Final(hash.data(), &sha256_ctx);

  std::stringstream ss;
  for (const auto& byte : hash) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }

  return ss.str();
}

// Resource utilities implementation
namespace resource_utils {

auto scan_resources(const std::filesystem::path& res_dir)
    -> tl::expected<std::vector<ResourceFile>, AndroidResourceError> {
  if (!std::filesystem::exists(res_dir)) {
    return tl::unexpected(AndroidResourceError::IoError);
  }

  std::vector<ResourceFile> resources;

  // Scan all subdirectories
  for (const auto& entry : std::filesystem::recursive_directory_iterator(res_dir)) {
    if (entry.is_regular_file()) {
      ResourceFile resource;
      resource.path = entry.path();
      resource.type = ResourceFile::get_type(entry.path());

      // Parse qualifiers from directory name
      if (entry.path().has_parent_path()) {
        auto dir_name = entry.path().parent_path().filename().string();
        resource.qualifiers = ResourceQualifiers::parse(dir_name);
      }

      // Compute hash
      auto hash_result = ResourceFile::compute_hash(entry.path());
      if (hash_result) {
        resource.content_hash = *hash_result;
      }

      resources.push_back(std::move(resource));
    }
  }

  return resources;
}

auto sort_resources(std::vector<ResourceFile>& resources) -> void {
  std::sort(resources.begin(), resources.end(), [](const ResourceFile& a, const ResourceFile& b) {
    // Sort by type, then qualifiers, then path
    if (a.type != b.type) {
      return static_cast<int>(a.type) < static_cast<int>(b.type);
    }

    auto a_qual = a.qualifiers.to_string();
    auto b_qual = b.qualifiers.to_string();
    if (a_qual != b_qual) {
      return a_qual < b_qual;
    }

    return a.path < b.path;
  });
}

auto filter_by_type(const std::vector<ResourceFile>& resources,
                    ResourceType type) -> std::vector<ResourceFile> {
  std::vector<ResourceFile> filtered;
  std::copy_if(resources.begin(), resources.end(), std::back_inserter(filtered),
               [type](const ResourceFile& r) { return r.type == type; });
  return filtered;
}

auto filter_by_qualifiers(const std::vector<ResourceFile>& resources,
                          const ResourceQualifiers& qualifiers) -> std::vector<ResourceFile> {
  std::vector<ResourceFile> filtered;
  std::copy_if(resources.begin(), resources.end(), std::back_inserter(filtered),
               [&qualifiers](const ResourceFile& r) {
                 return r.qualifiers.locale == qualifiers.locale &&
                        r.qualifiers.region == qualifiers.region &&
                        r.qualifiers.density == qualifiers.density;
               });
  return filtered;
}

auto resource_type_to_string(ResourceType type) -> std::string {
  switch (type) {
  case ResourceType::Values:
    return "values";
  case ResourceType::Layout:
    return "layout";
  case ResourceType::Drawable:
    return "drawable";
  case ResourceType::Mipmap:
    return "mipmap";
  case ResourceType::Raw:
    return "raw";
  case ResourceType::Xml:
    return "xml";
  case ResourceType::Anim:
    return "anim";
  case ResourceType::Animator:
    return "animator";
  case ResourceType::Color:
    return "color";
  case ResourceType::Menu:
    return "menu";
  case ResourceType::Unknown:
    return "unknown";
  }
  return "unknown";
}

auto string_to_resource_type(const std::string& type_str) -> ResourceType {
  if (type_str == "values")
    return ResourceType::Values;
  if (type_str == "layout")
    return ResourceType::Layout;
  if (type_str == "drawable")
    return ResourceType::Drawable;
  if (type_str == "mipmap")
    return ResourceType::Mipmap;
  if (type_str == "raw")
    return ResourceType::Raw;
  if (type_str == "xml")
    return ResourceType::Xml;
  if (type_str == "anim")
    return ResourceType::Anim;
  if (type_str == "animator")
    return ResourceType::Animator;
  if (type_str == "color")
    return ResourceType::Color;
  if (type_str == "menu")
    return ResourceType::Menu;
  return ResourceType::Unknown;
}

auto density_to_string(ResourceDensity density) -> std::string {
  switch (density) {
  case ResourceDensity::LDPI:
    return "ldpi";
  case ResourceDensity::MDPI:
    return "mdpi";
  case ResourceDensity::HDPI:
    return "hdpi";
  case ResourceDensity::XHDPI:
    return "xhdpi";
  case ResourceDensity::XXHDPI:
    return "xxhdpi";
  case ResourceDensity::XXXHDPI:
    return "xxxhdpi";
  case ResourceDensity::NODPI:
    return "nodpi";
  case ResourceDensity::TVDPI:
    return "tvdpi";
  case ResourceDensity::ANYDPI:
    return "anydpi";
  case ResourceDensity::None:
    return "";
  }
  return "";
}

auto string_to_density(const std::string& density_str) -> ResourceDensity {
  if (density_str == "ldpi")
    return ResourceDensity::LDPI;
  if (density_str == "mdpi")
    return ResourceDensity::MDPI;
  if (density_str == "hdpi")
    return ResourceDensity::HDPI;
  if (density_str == "xhdpi")
    return ResourceDensity::XHDPI;
  if (density_str == "xxhdpi")
    return ResourceDensity::XXHDPI;
  if (density_str == "xxxhdpi")
    return ResourceDensity::XXXHDPI;
  if (density_str == "nodpi")
    return ResourceDensity::NODPI;
  if (density_str == "tvdpi")
    return ResourceDensity::TVDPI;
  if (density_str == "anydpi")
    return ResourceDensity::ANYDPI;
  return ResourceDensity::None;
}

} // namespace resource_utils

// Incremental resource compilation support
namespace resource_incremental {

auto compute_resource_hash(const std::filesystem::path& resource_path) -> std::string {
  auto hash_result = ResourceFile::compute_hash(resource_path);
  return hash_result.value_or("");
}

auto has_resource_changed(const std::filesystem::path& resource_path,
                          const std::string& cached_hash) -> bool {
  auto current_hash = compute_resource_hash(resource_path);
  return current_hash != cached_hash;
}

auto save_compilation_state(const std::filesystem::path& state_file,
                            const Aapt2CompileConfig& config,
                            const std::string& compilation_hash) -> bool {
  std::ofstream file(state_file);
  if (!file)
    return false;

  file << "compilation_hash=" << compilation_hash << "\n";
  file << "timestamp=" << std::chrono::system_clock::now().time_since_epoch().count() << "\n";

  for (const auto& resource : config.resources) {
    file << "resource=" << resource.path.string() << "," << resource.content_hash << "\n";
  }

  return true;
}

auto load_compilation_state(const std::filesystem::path& state_file)
    -> std::optional<CompilationState> {
  if (!std::filesystem::exists(state_file)) {
    return std::nullopt;
  }

  std::ifstream file(state_file);
  if (!file)
    return std::nullopt;

  CompilationState state;
  std::string line;

  while (std::getline(file, line)) {
    auto pos = line.find('=');
    if (pos == std::string::npos)
      continue;

    auto key = line.substr(0, pos);
    auto value = line.substr(pos + 1);

    if (key == "compilation_hash") {
      state.compilation_hash = value;
    } else if (key == "timestamp") {
      try {
        auto count = std::stoll(value);
        state.timestamp =
            std::chrono::system_clock::time_point(std::chrono::system_clock::duration(count));
      } catch (...) {
      }
    } else if (key == "resource") {
      auto comma_pos = value.find(',');
      if (comma_pos != std::string::npos) {
        auto path = value.substr(0, comma_pos);
        auto hash = value.substr(comma_pos + 1);
        state.resource_hashes[path] = hash;
      }
    }
  }

  return state;
}

} // namespace resource_incremental

} // namespace horcrux::core
