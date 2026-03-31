// Horcrux - Sandbox Enforcement Regression Tests
// Copyright (C) 2026 Horcrux Project Contributors
// Licensed under the MIT License
//
// Release-critical regression suite: verifies that the sandbox policy model
// correctly enforces hermeticity constraints, produces stable fingerprints for
// cache-key computation, and that policy changes invalidate prior keys.

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/sandbox_policy.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// Regression: SandboxMode round-trips through string conversion
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxRegressionTest, ModeRoundTripsOff) {
  auto result = sandbox_mode_from_string(to_string(SandboxMode::Off));
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, SandboxMode::Off);
}

TEST(SandboxRegressionTest, ModeRoundTripsBalanced) {
  auto result = sandbox_mode_from_string(to_string(SandboxMode::Balanced));
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, SandboxMode::Balanced);
}

TEST(SandboxRegressionTest, ModeRoundTripsStrict) {
  auto result = sandbox_mode_from_string(to_string(SandboxMode::Strict));
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, SandboxMode::Strict);
}

TEST(SandboxRegressionTest, InvalidModeStringReturnsError) {
  auto result = sandbox_mode_from_string("turbo");
  EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: policy fingerprint is deterministic
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxRegressionTest, FingerprintIsDeterministic) {
  auto p = SandboxPolicy::default_hermetic();
  EXPECT_EQ(p.fingerprint(), p.fingerprint());
}

TEST(SandboxRegressionTest, IdenticalPoliciesProduceSameFingerprint) {
  auto p1 = SandboxPolicy::default_hermetic();
  auto p2 = SandboxPolicy::default_hermetic();
  EXPECT_EQ(p1.fingerprint(), p2.fingerprint());
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: policy changes produce different fingerprints
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxRegressionTest, ModeChangeDifferentiatesFingerprint) {
  SandboxPolicy balanced = SandboxPolicy::default_hermetic();
  SandboxPolicy strict = SandboxPolicy::strict_hermetic();
  EXPECT_NE(balanced.fingerprint(), strict.fingerprint());
}

TEST(SandboxRegressionTest, NetworkPolicyChangeDifferentiatesFingerprint) {
  SandboxPolicy deny = SandboxPolicy::default_hermetic();
  deny.network = NetworkPolicy::Deny;

  SandboxPolicy allow = SandboxPolicy::default_hermetic();
  allow.network = NetworkPolicy::Allow;

  EXPECT_NE(deny.fingerprint(), allow.fingerprint());
}

TEST(SandboxRegressionTest, EnvVarAllowlistChangeDifferentiatesFingerprint) {
  SandboxPolicy base = SandboxPolicy::default_hermetic();
  SandboxPolicy extended = SandboxPolicy::default_hermetic();
  extended.allowed_env_vars.push_back("MY_EXTRA_VAR");

  EXPECT_NE(base.fingerprint(), extended.fingerprint());
}

TEST(SandboxRegressionTest, PathAllowlistChangeDifferentiatesFingerprint) {
  SandboxPolicy base = SandboxPolicy::default_hermetic();
  SandboxPolicy with_path = SandboxPolicy::default_hermetic();
  with_path.allowed_paths.push_back({"/tmp/extra", false});

  EXPECT_NE(base.fingerprint(), with_path.fingerprint());
}

TEST(SandboxRegressionTest, ResourceLimitChangeDifferentiatesFingerprint) {
  SandboxPolicy base = SandboxPolicy::default_hermetic();
  SandboxPolicy with_limit = SandboxPolicy::default_hermetic();
  with_limit.limits.max_memory_bytes = 512 * 1024 * 1024u;

  EXPECT_NE(base.fingerprint(), with_limit.fingerprint());
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: equality operators are consistent with fingerprints
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxRegressionTest, EqualPoliciesAreEqual) {
  auto p1 = SandboxPolicy::default_hermetic();
  auto p2 = SandboxPolicy::default_hermetic();
  EXPECT_EQ(p1, p2);
  EXPECT_FALSE(p1 != p2);
}

TEST(SandboxRegressionTest, DifferentModePoliciesAreNotEqual) {
  auto off = SandboxPolicy::off();
  auto balanced = SandboxPolicy::default_hermetic();
  EXPECT_NE(off, balanced);
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: default hermetic policy has expected characteristics
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxRegressionTest, DefaultHermeticHasBalancedMode) {
  auto p = SandboxPolicy::default_hermetic();
  EXPECT_EQ(p.mode, SandboxMode::Balanced);
}

TEST(SandboxRegressionTest, DefaultHermeticDeniesNetwork) {
  auto p = SandboxPolicy::default_hermetic();
  EXPECT_EQ(p.network, NetworkPolicy::Deny);
}

TEST(SandboxRegressionTest, OffPolicyHasOffMode) {
  auto p = SandboxPolicy::off();
  EXPECT_EQ(p.mode, SandboxMode::Off);
}

TEST(SandboxRegressionTest, StrictPolicyHasStrictMode) {
  auto p = SandboxPolicy::strict_hermetic();
  EXPECT_EQ(p.mode, SandboxMode::Strict);
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: PolicyViolation kind string conversions are stable
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxRegressionTest, PolicyViolationKindStringsAreStable) {
  EXPECT_EQ(to_string(PolicyViolation::Kind::UndeclaredEnvVar), "undeclared_env_var");
  EXPECT_EQ(to_string(PolicyViolation::Kind::UndeclaredPath), "undeclared_path");
  EXPECT_EQ(to_string(PolicyViolation::Kind::NetworkAccess), "network_access");
  EXPECT_EQ(to_string(PolicyViolation::Kind::UndeclaredTool), "undeclared_tool");
  EXPECT_EQ(to_string(PolicyViolation::Kind::ResourceLimitBreached), "resource_limit_breached");
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: NetworkPolicy string conversions are stable
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxRegressionTest, NetworkPolicyStringsAreStable) {
  EXPECT_EQ(to_string(NetworkPolicy::Deny), "deny");
  EXPECT_EQ(to_string(NetworkPolicy::AllowList), "allowlist");
  EXPECT_EQ(to_string(NetworkPolicy::Allow), "allow");
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: timeout limit is reflected in fingerprint
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxRegressionTest, TimeoutLimitDifferentiatesFingerprint) {
  SandboxPolicy base = SandboxPolicy::default_hermetic();
  SandboxPolicy timed = SandboxPolicy::default_hermetic();
  timed.limits.timeout = std::chrono::seconds(120);

  EXPECT_NE(base.fingerprint(), timed.fingerprint());
}

} // namespace horcrux::core::test
