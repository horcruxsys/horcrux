// Horcrux - Sandbox Policy Model
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "local_cache.h"

namespace horcrux::core {

/// @brief Sandbox enforcement level
enum class SandboxMode {
  Off,      ///< No sandboxing; actions run directly on host
  Balanced, ///< Env-var filtering + path allowlists; no kernel namespace isolation
  Strict,   ///< Full isolation: namespaces + read-only input mounts + network deny
};

/// @brief Convert SandboxMode to string
[[nodiscard]] auto to_string(SandboxMode mode) -> std::string;

/// @brief Parse SandboxMode from string ("off", "balanced", "strict")
/// @return Expected SandboxMode or error string
[[nodiscard]] auto sandbox_mode_from_string(std::string_view s)
    -> tl::expected<SandboxMode, std::string>;

/// @brief Network access policy for hermetic actions
enum class NetworkPolicy {
  Deny,        ///< All network access is denied (default for hermetic builds)
  AllowList,   ///< Only explicitly allowed hosts/rules are permitted
  Allow,       ///< Unrestricted network access (non-hermetic; emits diagnostic)
};

/// @brief Convert NetworkPolicy to string
[[nodiscard]] auto to_string(NetworkPolicy policy) -> std::string;

/// @brief Resource limits that can be applied to sandboxed actions
struct ResourceLimits {
  std::optional<size_t> max_memory_bytes;      ///< Hard memory cap (0 = unlimited)
  std::optional<std::chrono::seconds> timeout; ///< Wall-clock timeout
  std::optional<uint32_t> max_processes;       ///< Max spawned child processes
};

/// @brief A single explicit host-path allowlist entry
struct PathAllowEntry {
  std::string host_path;   ///< Absolute host path permitted for read access
  bool writable = false;   ///< Whether write access is also permitted
};

/// @brief Complete policy configuration for hermetic build execution
///
/// SandboxPolicy captures every execution parameter that affects hermeticity.
/// It is hashed into cache keys so that cache hits are only valid when the
/// policy that produced an artifact matches the current policy.
struct SandboxPolicy {
  SandboxMode mode = SandboxMode::Balanced;
  NetworkPolicy network = NetworkPolicy::Deny;

  /// Explicit env-var allowlist; variables not in this list are stripped
  /// when mode >= Balanced.  Empty means "allow all" for Off mode only.
  std::vector<std::string> allowed_env_vars;

  /// Host paths that build actions are permitted to read (and optionally write)
  std::vector<PathAllowEntry> allowed_paths;

  /// Network allow rules (host[:port] strings); only used when
  /// network == NetworkPolicy::AllowList
  std::vector<std::string> allowed_network_rules;

  /// Per-action resource limits
  ResourceLimits limits;

  /// Whether undeclared accesses produce a hard error (true) or a warning (false)
  bool fail_on_violation = true;

  /// Default hermetic policy: Balanced + Deny + standard env vars
  [[nodiscard]] static auto default_hermetic() -> SandboxPolicy;

  /// Permissive policy used when --sandbox=off is requested
  [[nodiscard]] static auto off() -> SandboxPolicy;

  /// Strict policy: full namespace isolation
  [[nodiscard]] static auto strict_hermetic() -> SandboxPolicy;

  /// Compute a stable SHA-256 fingerprint of this policy.
  /// The fingerprint is incorporated into cache keys so that cache hits are
  /// only valid when the policy has not changed.
  [[nodiscard]] auto fingerprint() const -> Hash;

  /// Returns true if two policies would produce the same fingerprint
  [[nodiscard]] auto operator==(const SandboxPolicy& other) const -> bool;
  [[nodiscard]] auto operator!=(const SandboxPolicy& other) const -> bool;
};

/// @brief Diagnostics emitted when a policy violation is detected
struct PolicyViolation {
  enum class Kind {
    UndeclaredEnvVar,   ///< Action read an env var not in the allowlist
    UndeclaredPath,     ///< Action accessed a host path not in the allowlist
    NetworkAccess,      ///< Action performed a network call that was denied
    UndeclaredTool,     ///< Action used a host tool not declared in rule metadata
    ResourceLimitBreached, ///< Action exceeded a resource limit
  };

  Kind kind;
  std::string description;
  std::optional<std::string> detail; ///< e.g., the offending path or env-var name
};

/// @brief Convert PolicyViolation::Kind to string
[[nodiscard]] auto to_string(PolicyViolation::Kind kind) -> std::string;

} // namespace horcrux::core
