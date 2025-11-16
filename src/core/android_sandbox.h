// Horcrux - Android Build Sandbox & Hermeticity
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#ifndef HORCRUX_CORE_ANDROID_SANDBOX_H_
#define HORCRUX_CORE_ANDROID_SANDBOX_H_

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

namespace horcrux::core {

/// @brief Error types for sandbox operations
enum class SandboxError {
  InvalidConfiguration,
  MountFailed,
  NamespaceCreationFailed,
  ProcessExecutionFailed,
  ViolationDetected,
  UnsupportedPlatform,
  IoError,
  UnknownError
};

// Convert error to string
auto to_string(SandboxError error) -> std::string;

/// @brief Mount rule type for sandbox filesystem isolation
enum class MountType {
  ReadOnly,  // Read-only bind mount
  ReadWrite, // Read-write bind mount (scratch directories)
  TmpFs,     // Temporary filesystem in memory
  Proc,      // /proc filesystem
  DevNull    // /dev/null device
};

/// @brief Individual mount rule for sandbox
struct MountRule {
  std::filesystem::path source; // Source path on host
  std::filesystem::path target; // Target path in sandbox
  MountType type;               // Mount type
  bool optional = false;        // If true, don't fail if source doesn't exist

  // Create a read-only mount rule
  static auto read_only(const std::filesystem::path& source,
                        const std::filesystem::path& target) -> MountRule;

  // Create a read-write mount rule (for scratch directories)
  static auto read_write(const std::filesystem::path& source,
                         const std::filesystem::path& target) -> MountRule;

  // Create a tmpfs mount rule
  static auto tmpfs(const std::filesystem::path& target) -> MountRule;
};

/// @brief Sandbox violation types
enum class ViolationType {
  UnauthorizedFileAccess,    // Accessed file outside allowed paths
  UnauthorizedNetworkAccess, // Attempted network access
  UnauthorizedProcessSpawn,  // Spawned unauthorized subprocess
  UnauthorizedWrite,         // Write to read-only location
  UnknownViolation
};

/// @brief Sandbox violation record
struct SandboxViolation {
  ViolationType type;
  std::string description;
  std::optional<std::filesystem::path> path;
  std::chrono::system_clock::time_point timestamp;
};

/// @brief Sandbox configuration
struct SandboxConfig {
  // Command to execute in sandbox
  std::filesystem::path executable;
  std::vector<std::string> arguments;

  // Working directory inside sandbox
  std::filesystem::path working_dir;

  // Mount rules for filesystem isolation
  std::vector<MountRule> mounts;

  // Environment variables
  std::vector<std::pair<std::string, std::string>> env_vars;

  // Isolation features
  bool enable_network_isolation = true; // Disable network access
  bool enable_pid_namespace = true;     // Isolate process tree
  bool enable_mount_namespace = true;   // Isolate filesystem mounts
  bool enable_ipc_namespace = true;     // Isolate IPC
  bool enable_uts_namespace = true;     // Isolate hostname

  // Violation detection
  bool detect_violations = true; // Enable violation detection
  bool fail_on_violation = true; // Fail build on violation

  // Resource limits
  std::optional<size_t> max_memory_bytes;      // Max memory usage
  std::optional<std::chrono::seconds> timeout; // Execution timeout

  // Debugging
  bool verbose = false; // Enable verbose output
};

/// @brief Sandbox execution result
struct SandboxResult {
  // Exit code of sandboxed process
  int exit_code;

  // Standard output from process
  std::string stdout_output;

  // Standard error from process
  std::string stderr_output;

  // Execution time
  std::chrono::milliseconds execution_time;

  // Detected violations (if any)
  std::vector<SandboxViolation> violations;

  // Success status
  bool success() const {
    return exit_code == 0 && violations.empty();
  }
};

/// @brief Android Build Sandbox - provides hermetic build execution
///
/// The AndroidSandbox ensures that Android build tools (kotlinc, javac, aapt2,
/// d8/r8, zipalign, apksigner) run in isolated environments with controlled
/// filesystem access.
///
/// Key features:
/// - Read-only SDK mounts
/// - Read-only source mounts
/// - Writable scratch directories
/// - Network isolation
/// - Process isolation using Linux namespaces
/// - Sandbox violation detection
///
/// Example usage:
/// @code
///   SandboxConfig config;
///   config.executable = "/usr/bin/javac";
///   config.arguments = {"-d", "output", "Main.java"};
///   config.working_dir = "/sandbox/work";
///
///   // Mount SDK as read-only
///   config.mounts.push_back(
///       MountRule::read_only("/sdk/android", "/sandbox/sdk"));
///
///   // Mount source as read-only
///   config.mounts.push_back(
///       MountRule::read_only("/project/src", "/sandbox/src"));
///
///   // Mount scratch as read-write
///   config.mounts.push_back(
///       MountRule::read_write("/tmp/build", "/sandbox/work"));
///
///   AndroidSandbox sandbox;
///   auto result = sandbox.execute(config);
///   if (!result) {
///     // Handle error
///   }
/// @endcode
class AndroidSandbox {
public:
  AndroidSandbox() = default;

  /// @brief Execute command in sandbox
  /// @param config Sandbox configuration
  /// @return Sandbox result or error
  auto execute(const SandboxConfig& config) -> tl::expected<SandboxResult, SandboxError>;

  /// @brief Check if sandboxing is supported on current platform
  /// @return true if sandboxing is supported
  static auto is_supported() -> bool;

  /// @brief Validate sandbox configuration
  /// @param config Configuration to validate
  /// @return Error if configuration is invalid
  static auto validate_config(const SandboxConfig& config) -> tl::expected<void, SandboxError>;

  /// @brief Create standard Android build sandbox configuration
  /// @param executable Build tool executable
  /// @param arguments Build tool arguments
  /// @param sdk_path Android SDK path (mounted read-only)
  /// @param source_path Source directory (mounted read-only)
  /// @param scratch_path Scratch directory (mounted read-write)
  /// @return Configured sandbox
  static auto create_android_build_sandbox(
      const std::filesystem::path& executable, const std::vector<std::string>& arguments,
      const std::filesystem::path& sdk_path, const std::filesystem::path& source_path,
      const std::filesystem::path& scratch_path) -> SandboxConfig;

private:
  /// @brief Execute in sandbox using Linux namespaces
  auto
  execute_with_namespaces(const SandboxConfig& config) -> tl::expected<SandboxResult, SandboxError>;

  /// @brief Execute in sandbox using basic isolation (fallback)
  auto execute_basic(const SandboxConfig& config) -> tl::expected<SandboxResult, SandboxError>;

  /// @brief Setup mount namespace
  auto setup_mounts(const std::vector<MountRule>& mounts) -> tl::expected<void, SandboxError>;

  /// @brief Detect sandbox violations from process output
  auto detect_violations(const std::string& stdout_output, const std::string& stderr_output,
                         const SandboxConfig& config) -> std::vector<SandboxViolation>;

  /// @brief Apply resource limits to process
  auto apply_resource_limits(const SandboxConfig& config) -> tl::expected<void, SandboxError>;
};

/// @brief RAII helper for sandbox cleanup
class SandboxGuard {
public:
  explicit SandboxGuard(const std::filesystem::path& scratch_dir);
  ~SandboxGuard();

  SandboxGuard(const SandboxGuard&) = delete;
  SandboxGuard& operator=(const SandboxGuard&) = delete;
  SandboxGuard(SandboxGuard&&) = default;
  SandboxGuard& operator=(SandboxGuard&&) = default;

private:
  std::filesystem::path scratch_dir_;
  bool cleanup_on_destroy_ = true;
};

} // namespace horcrux::core

#endif // HORCRUX_CORE_ANDROID_SANDBOX_H_
