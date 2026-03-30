// Horcrux - Deterministic Environment Contract Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "hermetic_env.h"

#include <algorithm>
#include <cstdlib>
#include <set>

// `environ` is POSIX-standard. On Windows, use GetEnvironmentStringsW instead.
#ifndef _WIN32
#include <unistd.h>
#endif

namespace horcrux::core {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Apply normalization overrides on top of the filtered map
void apply_normalizations(std::unordered_map<std::string, std::string>& env,
                          std::string_view output_root) {
  // Normalize locale
  env["LANG"] = std::string(HermeticEnv::kNormalizedLocale);
  env["LC_ALL"] = std::string(HermeticEnv::kNormalizedLocale);
  env["LC_CTYPE"] = std::string(HermeticEnv::kNormalizedLocale);

  // Normalize timezone
  env["TZ"] = std::string(HermeticEnv::kNormalizedTimezone);

  // Normalize temp directory
  env["TMPDIR"] = std::string(HermeticEnv::kNormalizedTmpDir);
  env["TMP"] = std::string(HermeticEnv::kNormalizedTmpDir);
  env["TEMP"] = std::string(HermeticEnv::kNormalizedTmpDir);

  // Inject Horcrux output root
  if (!output_root.empty()) {
    env["HORCRUX_OUTPUT_ROOT"] = std::string(output_root);
  }
}

/// Convert map to sorted pairs
auto to_sorted_pairs(const std::unordered_map<std::string, std::string>& map)
    -> std::vector<std::pair<std::string, std::string>> {
  std::vector<std::pair<std::string, std::string>> pairs(map.begin(), map.end());
  std::sort(pairs.begin(), pairs.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  return pairs;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Constructors / factory methods
// ─────────────────────────────────────────────────────────────────────────────

HermeticEnv::HermeticEnv(std::vector<std::pair<std::string, std::string>> env)
    : env_(std::move(env)) {}

auto HermeticEnv::build(const SandboxPolicy& policy,
                        std::string_view output_root) -> HermeticEnv {
  // Collect host environment
  std::unordered_map<std::string, std::string> host_map;
#ifndef _WIN32
  // `environ` is POSIX-standard (available via <unistd.h> on Linux/macOS)
  if (environ) {
    for (char** ep = environ; *ep != nullptr; ++ep) {
      std::string_view entry(*ep);
      auto eq = entry.find('=');
      if (eq != std::string_view::npos) {
        host_map.emplace(std::string(entry.substr(0, eq)),
                         std::string(entry.substr(eq + 1)));
      }
    }
  }
#endif
  return build_from_map(policy, host_map, output_root);
}

auto HermeticEnv::build_from_map(
    const SandboxPolicy& policy,
    const std::unordered_map<std::string, std::string>& host_env,
    std::string_view output_root) -> HermeticEnv {
  std::unordered_map<std::string, std::string> filtered;

  if (policy.mode == SandboxMode::Off) {
    // In Off mode pass through the entire host environment
    filtered = host_env;
  } else {
    // Balanced / Strict: only copy allowed vars
    for (const auto& key : policy.allowed_env_vars) {
      auto it = host_env.find(key);
      if (it != host_env.end()) {
        filtered[key] = it->second;
      }
    }
  }

  // Apply normalization overrides (these always win)
  apply_normalizations(filtered, output_root);

  return HermeticEnv(to_sorted_pairs(filtered));
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

auto HermeticEnv::as_sorted_pairs() const
    -> const std::vector<std::pair<std::string, std::string>>& {
  return env_;
}

auto HermeticEnv::get(std::string_view name) const -> std::optional<std::string> {
  // Binary search since env_ is sorted
  auto it = std::lower_bound(env_.begin(), env_.end(), std::pair<std::string, std::string>{
                                                            std::string(name), {}});
  if (it != env_.end() && it->first == name) {
    return it->second;
  }
  return std::nullopt;
}

auto HermeticEnv::size() const -> size_t {
  return env_.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// Canonical input ordering helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {
/// Normalize a single path string: remove trailing slash, collapse redundant '.'
auto normalize_path_string(std::string p) -> std::string {
  // Remove trailing slash (except root "/")
  while (p.size() > 1 && p.back() == '/') {
    p.pop_back();
  }
  return p;
}
} // namespace

auto HermeticEnv::sort_inputs(std::vector<std::string> inputs) -> std::vector<std::string> {
  // Normalize
  for (auto& p : inputs) {
    p = normalize_path_string(std::move(p));
  }
  // Deduplicate and sort
  std::sort(inputs.begin(), inputs.end());
  inputs.erase(std::unique(inputs.begin(), inputs.end()), inputs.end());
  return inputs;
}

auto HermeticEnv::sort_includes(std::vector<std::string> includes) -> std::vector<std::string> {
  return sort_inputs(std::move(includes));
}

auto HermeticEnv::sort_deps(std::vector<std::string> deps) -> std::vector<std::string> {
  std::sort(deps.begin(), deps.end());
  deps.erase(std::unique(deps.begin(), deps.end()), deps.end());
  return deps;
}

} // namespace horcrux::core
