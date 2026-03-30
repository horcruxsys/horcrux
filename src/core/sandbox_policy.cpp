// Horcrux - Sandbox Policy Model Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "sandbox_policy.h"

#include <algorithm>
#include <sstream>

namespace horcrux::core {

auto to_string(SandboxMode mode) -> std::string {
  switch (mode) {
  case SandboxMode::Off:
    return "off";
  case SandboxMode::Balanced:
    return "balanced";
  case SandboxMode::Strict:
    return "strict";
  }
  return "unknown";
}

auto sandbox_mode_from_string(std::string_view s) -> tl::expected<SandboxMode, std::string> {
  if (s == "off")
    return SandboxMode::Off;
  if (s == "balanced")
    return SandboxMode::Balanced;
  if (s == "strict")
    return SandboxMode::Strict;
  return tl::unexpected(std::string("Unknown sandbox mode: ") + std::string(s));
}

auto to_string(NetworkPolicy policy) -> std::string {
  switch (policy) {
  case NetworkPolicy::Deny:
    return "deny";
  case NetworkPolicy::AllowList:
    return "allowlist";
  case NetworkPolicy::Allow:
    return "allow";
  }
  return "unknown";
}

auto to_string(PolicyViolation::Kind kind) -> std::string {
  switch (kind) {
  case PolicyViolation::Kind::UndeclaredEnvVar:
    return "undeclared_env_var";
  case PolicyViolation::Kind::UndeclaredPath:
    return "undeclared_path";
  case PolicyViolation::Kind::NetworkAccess:
    return "network_access";
  case PolicyViolation::Kind::UndeclaredTool:
    return "undeclared_tool";
  case PolicyViolation::Kind::ResourceLimitBreached:
    return "resource_limit_breached";
  }
  return "unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory helpers
// ─────────────────────────────────────────────────────────────────────────────

auto SandboxPolicy::default_hermetic() -> SandboxPolicy {
  SandboxPolicy p;
  p.mode = SandboxMode::Balanced;
  p.network = NetworkPolicy::Deny;
  p.fail_on_violation = true;

  // Canonical set of env vars permitted in hermetic builds
  p.allowed_env_vars = {
      "HOME",
      "PATH",
      "USER",
      "LOGNAME",
      "TMPDIR",
      "TMP",
      "TEMP",
      "LANG",
      "LC_ALL",
      "LC_CTYPE",
      "TZ",
      // Horcrux-specific overrides
      "HORCRUX_CACHE_DIR",
      "HORCRUX_OUTPUT_ROOT",
  };
  return p;
}

auto SandboxPolicy::off() -> SandboxPolicy {
  SandboxPolicy p;
  p.mode = SandboxMode::Off;
  p.network = NetworkPolicy::Allow;
  p.fail_on_violation = false;
  // No env-var or path restrictions in Off mode
  return p;
}

auto SandboxPolicy::strict_hermetic() -> SandboxPolicy {
  SandboxPolicy p = default_hermetic();
  p.mode = SandboxMode::Strict;
  // Strict mode keeps the same env allowlist but uses kernel namespace isolation
  return p;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fingerprint
// ─────────────────────────────────────────────────────────────────────────────

auto SandboxPolicy::fingerprint() const -> Hash {
  // Serialize the policy to a canonical byte string and hash it.
  std::ostringstream oss;
  oss << "mode=" << to_string(mode) << '\n';
  oss << "network=" << to_string(network) << '\n';
  oss << "fail_on_violation=" << (fail_on_violation ? "1" : "0") << '\n';

  // Env vars – sort for stability
  std::vector<std::string> sorted_env = allowed_env_vars;
  std::sort(sorted_env.begin(), sorted_env.end());
  for (const auto& v : sorted_env) {
    oss << "env=" << v << '\n';
  }

  // Paths – sort for stability
  std::vector<PathAllowEntry> sorted_paths = allowed_paths;
  std::sort(
      sorted_paths.begin(), sorted_paths.end(),
      [](const PathAllowEntry& a, const PathAllowEntry& b) { return a.host_path < b.host_path; });
  for (const auto& p : sorted_paths) {
    oss << "path=" << p.host_path << ':' << (p.writable ? "rw" : "ro") << '\n';
  }

  // Network rules – sort for stability
  std::vector<std::string> sorted_net = allowed_network_rules;
  std::sort(sorted_net.begin(), sorted_net.end());
  for (const auto& r : sorted_net) {
    oss << "net=" << r << '\n';
  }

  // Resource limits
  if (limits.max_memory_bytes) {
    oss << "mem=" << *limits.max_memory_bytes << '\n';
  }
  if (limits.timeout) {
    oss << "timeout=" << limits.timeout->count() << '\n';
  }
  if (limits.max_processes) {
    oss << "procs=" << *limits.max_processes << '\n';
  }

  const std::string canonical = oss.str();
  const std::vector<uint8_t> bytes(canonical.begin(), canonical.end());
  return compute_sha256(bytes);
}

auto SandboxPolicy::operator==(const SandboxPolicy& other) const -> bool {
  return fingerprint() == other.fingerprint();
}

auto SandboxPolicy::operator!=(const SandboxPolicy& other) const -> bool {
  return !(*this == other);
}

} // namespace horcrux::core
