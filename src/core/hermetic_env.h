// Horcrux - Deterministic Environment Contract
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "sandbox_policy.h"

namespace horcrux::core {

/// @brief A filtered, normalized set of environment variables for hermetic actions
///
/// HermeticEnv builds the environment that will be passed to a sandboxed
/// build action. It:
///   - Strips all host env vars not in the allowlist.
///   - Overrides locale, timezone, and temp directory to stable values.
///   - Provides deterministic PATH mapping.
///   - Ensures canonical input ordering for argv, include lists, etc.
class HermeticEnv {
public:
  /// @brief Build a hermetic environment from the host environment
  ///
  /// Reads the host environment, keeps only vars in policy.allowed_env_vars,
  /// and applies normalization overrides.
  ///
  /// @param policy  Active sandbox policy (determines allowlist)
  /// @param output_root Absolute path used as HORCRUX_OUTPUT_ROOT
  /// @return Populated HermeticEnv
  [[nodiscard]] static auto build(const SandboxPolicy& policy,
                                  std::string_view output_root) -> HermeticEnv;

  /// @brief Build a hermetic environment from an explicit variable map
  ///
  /// Useful in tests and for deterministic replay.
  [[nodiscard]] static auto
  build_from_map(const SandboxPolicy& policy,
                 const std::unordered_map<std::string, std::string>& host_env,
                 std::string_view output_root) -> HermeticEnv;

  // ── Normalisation constants ──────────────────────────────────────────────

  /// Normalized locale injected into every hermetic environment
  static constexpr std::string_view kNormalizedLocale = "en_US.UTF-8";

  /// Normalized timezone injected into every hermetic environment
  static constexpr std::string_view kNormalizedTimezone = "UTC";

  /// Normalized temp directory root injected into every hermetic environment
  static constexpr std::string_view kNormalizedTmpDir = "/tmp/horcrux-sandbox";

  // ── Accessors ────────────────────────────────────────────────────────────

  /// @brief Returns the filtered+normalized env as a sorted key=value vector
  ///
  /// The vector is stable-sorted by key name so the result is deterministic
  /// across calls even when the host environment ordering differs.
  [[nodiscard]] auto as_sorted_pairs() const
      -> const std::vector<std::pair<std::string, std::string>>&;

  /// @brief Look up a single environment variable
  [[nodiscard]] auto get(std::string_view name) const -> std::optional<std::string>;

  /// @brief Returns the number of env vars in the hermetic environment
  [[nodiscard]] auto size() const -> size_t;

  // ── Canonical input ordering ─────────────────────────────────────────────

  /// @brief Sort and deduplicate a list of source paths canonically
  ///
  /// Canonically sorted means:
  ///   1. All paths are normalized (trailing slash removed, `.` removed).
  ///   2. Sorted lexicographically.
  ///   3. Duplicates removed.
  ///
  /// Use this before hashing or passing to a compiler to ensure determinism.
  [[nodiscard]] static auto sort_inputs(std::vector<std::string> inputs) -> std::vector<std::string>;

  /// @brief Sort and deduplicate include/classpath lists canonically
  [[nodiscard]] static auto sort_includes(std::vector<std::string> includes)
      -> std::vector<std::string>;

  /// @brief Sort and deduplicate a deps closure canonically (by label string)
  [[nodiscard]] static auto sort_deps(std::vector<std::string> deps) -> std::vector<std::string>;

private:
  explicit HermeticEnv(std::vector<std::pair<std::string, std::string>> env);

  std::vector<std::pair<std::string, std::string>> env_; ///< Sorted key=value pairs
};

} // namespace horcrux::core
