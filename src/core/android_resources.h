// Horcrux - Android Resource Processing (AAPT2 Pipeline)
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#ifndef HORCRUX_CORE_ANDROID_RESOURCES_H_
#define HORCRUX_CORE_ANDROID_RESOURCES_H_

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <tl/expected.hpp>

#include "android_toolchain.h"

namespace horcrux::core {

// Error types for Android resource processing
enum class AndroidResourceError {
  Aapt2NotFound,
  InvalidResourceFile,
  InvalidManifest,
  CompilationFailed,
  LinkingFailed,
  MergingFailed,
  InvalidConfiguration,
  IoError,
  UnknownError
};

// Convert error to string
auto to_string(AndroidResourceError error) -> std::string;

// Resource type enumeration
enum class ResourceType {
  Values,   // strings, colors, dimensions, etc. in res/values/
  Layout,   // XML layouts in res/layout/
  Drawable, // Drawables in res/drawable/
  Mipmap,   // Mipmaps in res/mipmap/
  Raw,      // Raw resources in res/raw/
  Xml,      // XML resources in res/xml/
  Anim,     // Animations in res/anim/
  Animator, // Property animations in res/animator/
  Color,    // Color state lists in res/color/
  Menu,     // Menu resources in res/menu/
  Unknown
};

// Resource density qualifier
enum class ResourceDensity {
  LDPI,    // ~120dpi
  MDPI,    // ~160dpi
  HDPI,    // ~240dpi
  XHDPI,   // ~320dpi
  XXHDPI,  // ~480dpi
  XXXHDPI, // ~640dpi
  NODPI,   // Density-independent
  TVDPI,   // ~213dpi (TV)
  ANYDPI,  // Any density
  None
};

// Resource configuration qualifiers
struct ResourceQualifiers {
  std::optional<std::string> locale; // e.g., "en", "es"
  std::optional<std::string> region; // e.g., "US", "GB"
  std::optional<ResourceDensity> density;
  std::optional<std::string> screen_size; // e.g., "small", "large"
  std::optional<std::string> orientation; // e.g., "port", "land"
  std::optional<std::string> night_mode;  // e.g., "night", "notnight"
  std::optional<int> api_level;           // e.g., 21, 28, 34

  // Parse qualifiers from directory name (e.g., "values-en-rUS-v21")
  static auto parse(const std::string& dir_name) -> ResourceQualifiers;

  // Convert to string (for sorting/hashing)
  auto to_string() const -> std::string;
};

// Individual resource file
struct ResourceFile {
  std::filesystem::path path;
  ResourceType type;
  ResourceQualifiers qualifiers;
  std::string content_hash; // SHA-256 hash for incremental builds

  // Parse resource type from path
  static auto get_type(const std::filesystem::path& path) -> ResourceType;

  // Compute content hash
  static auto compute_hash(const std::filesystem::path& path)
      -> tl::expected<std::string, AndroidResourceError>;
};

// Android manifest configuration
struct AndroidManifest {
  std::filesystem::path path;
  std::string package_name;
  std::string version_name;
  int version_code;
  int min_sdk_version;
  int target_sdk_version;
  std::optional<int> max_sdk_version;
  std::string content_hash;

  // Parse manifest XML
  static auto parse(const std::filesystem::path& manifest_path)
      -> tl::expected<AndroidManifest, AndroidResourceError>;
};

// AAPT2 compile configuration
struct Aapt2CompileConfig {
  std::vector<ResourceFile> resources;      // Resources to compile
  std::filesystem::path output_dir;         // Output directory for flat files
  bool incremental = true;                  // Enable incremental compilation
  bool verbose = false;                     // Enable verbose output
  std::vector<std::string> additional_args; // Additional AAPT2 arguments
};

// AAPT2 compile result
struct Aapt2CompileResult {
  std::vector<std::filesystem::path> compiled_files; // .flat files
  std::chrono::milliseconds compilation_time;
  std::string compilation_hash; // Merkle hash of entire compilation
};

// AAPT2 link configuration
struct Aapt2LinkConfig {
  std::vector<std::filesystem::path> compiled_resources; // .flat files
  std::filesystem::path manifest;                        // AndroidManifest.xml
  std::filesystem::path output_apk;                      // Output APK (resources.ap_)
  std::optional<std::filesystem::path> r_java_output;    // R.java output dir
  std::optional<std::filesystem::path> proguard_output;  // ProGuard rules output
  std::filesystem::path android_jar;                     // android.jar for linking
  std::vector<std::filesystem::path> overlays;           // Resource overlays
  std::optional<std::string> package_name;               // Override package name
  bool auto_add_overlay = false;                         // Auto-add overlay resources
  bool verbose = false;                                  // Enable verbose output
  std::vector<std::string> additional_args;              // Additional AAPT2 arguments
};

// AAPT2 link result
struct Aapt2LinkResult {
  std::filesystem::path output_apk;              // resources.ap_
  std::optional<std::filesystem::path> r_jar;    // R.jar
  std::optional<std::filesystem::path> proguard; // proguard.txt
  std::chrono::milliseconds link_time;
  std::string link_hash; // Hash of link operation
};

// Resource merging configuration
struct ResourceMergeConfig {
  std::vector<std::filesystem::path> resource_dirs; // Directories to merge
  std::filesystem::path output_dir;                 // Merged output directory
  bool deterministic = true;                        // Deterministic merge (sorted)
  bool verbose = false;
};

// Resource merging result
struct ResourceMergeResult {
  std::filesystem::path merged_dir;
  std::vector<ResourceFile> merged_resources;
  std::chrono::milliseconds merge_time;
  std::string merge_hash;
};

// Manifest merging configuration
struct ManifestMergeConfig {
  std::filesystem::path main_manifest;                  // Main AndroidManifest.xml
  std::vector<std::filesystem::path> library_manifests; // Library manifests
  std::filesystem::path output_manifest;                // Merged manifest output
  bool verbose = false;
};

// Manifest merging result
struct ManifestMergeResult {
  std::filesystem::path merged_manifest;
  std::chrono::milliseconds merge_time;
  std::string merge_hash;
};

// Main Android resource processor class
class AndroidResourceProcessor {
public:
  explicit AndroidResourceProcessor(const AndroidToolchain& toolchain);

  // AAPT2 compile: res/*.xml -> compiled flat files
  auto compile(const Aapt2CompileConfig& config)
      -> tl::expected<Aapt2CompileResult, AndroidResourceError>;

  // AAPT2 link: flat files + manifest -> resources.ap_ + R.jar
  auto link(const Aapt2LinkConfig& config) -> tl::expected<Aapt2LinkResult, AndroidResourceError>;

  // Resource merging: multiple resource dirs -> single merged dir
  auto merge_resources(const ResourceMergeConfig& config)
      -> tl::expected<ResourceMergeResult, AndroidResourceError>;

  // Manifest merging: multiple manifests -> single merged manifest
  auto merge_manifests(const ManifestMergeConfig& config)
      -> tl::expected<ManifestMergeResult, AndroidResourceError>;

  // Complete pipeline: resources + manifest -> R.jar + resources.ap_
  struct PipelineConfig {
    std::vector<std::filesystem::path> resource_dirs;
    std::filesystem::path manifest;
    std::filesystem::path output_dir;
    std::filesystem::path android_jar;
    std::optional<std::string> package_name;
    bool incremental = true;
    bool verbose = false;
  };

  struct PipelineResult {
    std::filesystem::path r_jar;
    std::filesystem::path resources_apk;
    std::filesystem::path processed_manifest;
    std::chrono::milliseconds total_time;
    std::string pipeline_hash;
  };

  auto process_pipeline(const PipelineConfig& config)
      -> tl::expected<PipelineResult, AndroidResourceError>;

  // Get AAPT2 path from toolchain
  auto get_aapt2_path() const -> std::optional<std::filesystem::path>;

  // Validate configuration
  static auto validate_compile_config(const Aapt2CompileConfig& config)
      -> tl::expected<void, AndroidResourceError>;

  static auto
  validate_link_config(const Aapt2LinkConfig& config) -> tl::expected<void, AndroidResourceError>;

  // Compute compilation hash (Merkle signature)
  static auto compute_compile_hash(const Aapt2CompileConfig& config) -> std::string;
  static auto compute_link_hash(const Aapt2LinkConfig& config) -> std::string;

private:
  const AndroidToolchain& toolchain_;
  std::optional<std::filesystem::path> aapt2_path_;

  // Find AAPT2 in build tools (prefers latest version with AAPT2)
  auto find_aapt2() const -> std::optional<std::filesystem::path>;

  // Execute AAPT2 command
  auto execute_aapt2(const std::vector<std::string>& args)
      -> tl::expected<std::string, AndroidResourceError>;
};

// Utility namespace for resource operations
namespace resource_utils {

// Scan directory for Android resources
auto scan_resources(const std::filesystem::path& res_dir)
    -> tl::expected<std::vector<ResourceFile>, AndroidResourceError>;

// Sort resources deterministically
auto sort_resources(std::vector<ResourceFile>& resources) -> void;

// Filter resources by type
auto filter_by_type(const std::vector<ResourceFile>& resources,
                    ResourceType type) -> std::vector<ResourceFile>;

// Filter resources by qualifiers
auto filter_by_qualifiers(const std::vector<ResourceFile>& resources,
                          const ResourceQualifiers& qualifiers) -> std::vector<ResourceFile>;

// Convert ResourceType to string
auto resource_type_to_string(ResourceType type) -> std::string;

// Convert string to ResourceType
auto string_to_resource_type(const std::string& type_str) -> ResourceType;

// Convert ResourceDensity to string
auto density_to_string(ResourceDensity density) -> std::string;

// Parse density from string
auto string_to_density(const std::string& density_str) -> ResourceDensity;

} // namespace resource_utils

// Incremental resource compilation support
namespace resource_incremental {

// Compute resource hash
auto compute_resource_hash(const std::filesystem::path& resource_path) -> std::string;

// Check if resource has changed
auto has_resource_changed(const std::filesystem::path& resource_path,
                          const std::string& cached_hash) -> bool;

// Compilation state for incremental builds
struct CompilationState {
  std::string compilation_hash;
  std::unordered_map<std::string, std::string> resource_hashes;
  std::chrono::system_clock::time_point timestamp;
};

// Save compilation state
auto save_compilation_state(const std::filesystem::path& state_file,
                            const Aapt2CompileConfig& config,
                            const std::string& compilation_hash) -> bool;

// Load compilation state
auto load_compilation_state(const std::filesystem::path& state_file)
    -> std::optional<CompilationState>;

} // namespace resource_incremental

} // namespace horcrux::core

#endif // HORCRUX_CORE_ANDROID_RESOURCES_H_
