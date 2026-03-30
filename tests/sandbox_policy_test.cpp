// Horcrux - Sandbox Policy Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/sandbox_policy.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// SandboxMode string conversions
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxModeTest, ToStringReturnsExpectedValues) {
  EXPECT_EQ(to_string(SandboxMode::Off), "off");
  EXPECT_EQ(to_string(SandboxMode::Balanced), "balanced");
  EXPECT_EQ(to_string(SandboxMode::Strict), "strict");
}

TEST(SandboxModeTest, FromStringParsesOff) {
  auto result = sandbox_mode_from_string("off");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, SandboxMode::Off);
}

TEST(SandboxModeTest, FromStringParsesBalanced) {
  auto result = sandbox_mode_from_string("balanced");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, SandboxMode::Balanced);
}

TEST(SandboxModeTest, FromStringParsesStrict) {
  auto result = sandbox_mode_from_string("strict");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, SandboxMode::Strict);
}

TEST(SandboxModeTest, FromStringRejectsUnknown) {
  auto result = sandbox_mode_from_string("unknown_mode");
  EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// NetworkPolicy string conversions
// ─────────────────────────────────────────────────────────────────────────────

TEST(NetworkPolicyTest, ToStringReturnsExpectedValues) {
  EXPECT_EQ(to_string(NetworkPolicy::Deny), "deny");
  EXPECT_EQ(to_string(NetworkPolicy::AllowList), "allowlist");
  EXPECT_EQ(to_string(NetworkPolicy::Allow), "allow");
}

// ─────────────────────────────────────────────────────────────────────────────
// PolicyViolation::Kind string conversions
// ─────────────────────────────────────────────────────────────────────────────

TEST(PolicyViolationTest, ToStringCoversAllKinds) {
  EXPECT_EQ(to_string(PolicyViolation::Kind::UndeclaredEnvVar), "undeclared_env_var");
  EXPECT_EQ(to_string(PolicyViolation::Kind::UndeclaredPath), "undeclared_path");
  EXPECT_EQ(to_string(PolicyViolation::Kind::NetworkAccess), "network_access");
  EXPECT_EQ(to_string(PolicyViolation::Kind::UndeclaredTool), "undeclared_tool");
  EXPECT_EQ(to_string(PolicyViolation::Kind::ResourceLimitBreached), "resource_limit_breached");
}

// ─────────────────────────────────────────────────────────────────────────────
// SandboxPolicy factory methods
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxPolicyTest, DefaultHermeticHasBalancedMode) {
  auto policy = SandboxPolicy::default_hermetic();
  EXPECT_EQ(policy.mode, SandboxMode::Balanced);
}

TEST(SandboxPolicyTest, DefaultHermeticDeniesNetwork) {
  auto policy = SandboxPolicy::default_hermetic();
  EXPECT_EQ(policy.network, NetworkPolicy::Deny);
}

TEST(SandboxPolicyTest, DefaultHermeticFailsOnViolation) {
  auto policy = SandboxPolicy::default_hermetic();
  EXPECT_TRUE(policy.fail_on_violation);
}

TEST(SandboxPolicyTest, DefaultHermeticHasStandardEnvVars) {
  auto policy = SandboxPolicy::default_hermetic();
  ASSERT_FALSE(policy.allowed_env_vars.empty());

  // Must contain the basic hermetic set
  const std::vector<std::string> required = {"HOME", "PATH", "TZ", "LANG"};
  for (const auto& var : required) {
    EXPECT_NE(std::find(policy.allowed_env_vars.begin(), policy.allowed_env_vars.end(), var),
              policy.allowed_env_vars.end())
        << "Missing expected env var: " << var;
  }
}

TEST(SandboxPolicyTest, OffModeAllowsNetwork) {
  auto policy = SandboxPolicy::off();
  EXPECT_EQ(policy.mode, SandboxMode::Off);
  EXPECT_EQ(policy.network, NetworkPolicy::Allow);
  EXPECT_FALSE(policy.fail_on_violation);
}

TEST(SandboxPolicyTest, StrictHermeticHasStrictMode) {
  auto policy = SandboxPolicy::strict_hermetic();
  EXPECT_EQ(policy.mode, SandboxMode::Strict);
  EXPECT_EQ(policy.network, NetworkPolicy::Deny);
  EXPECT_TRUE(policy.fail_on_violation);
}

// ─────────────────────────────────────────────────────────────────────────────
// SandboxPolicy fingerprint
// ─────────────────────────────────────────────────────────────────────────────

TEST(SandboxPolicyTest, SamePolicyProducesSameFingerprint) {
  auto p1 = SandboxPolicy::default_hermetic();
  auto p2 = SandboxPolicy::default_hermetic();
  EXPECT_EQ(p1.fingerprint(), p2.fingerprint());
}

TEST(SandboxPolicyTest, DifferentModesProduceDifferentFingerprints) {
  auto balanced = SandboxPolicy::default_hermetic();
  auto strict = SandboxPolicy::strict_hermetic();
  EXPECT_NE(balanced.fingerprint(), strict.fingerprint());
}

TEST(SandboxPolicyTest, DifferentNetworkPoliciesProduceDifferentFingerprints) {
  auto p1 = SandboxPolicy::default_hermetic();
  p1.network = NetworkPolicy::Allow;
  auto p2 = SandboxPolicy::default_hermetic();
  p2.network = NetworkPolicy::Deny;
  EXPECT_NE(p1.fingerprint(), p2.fingerprint());
}

TEST(SandboxPolicyTest, AddingEnvVarChangesFingerprint) {
  auto p1 = SandboxPolicy::default_hermetic();
  auto p2 = SandboxPolicy::default_hermetic();
  p2.allowed_env_vars.push_back("MY_CUSTOM_VAR");
  EXPECT_NE(p1.fingerprint(), p2.fingerprint());
}

TEST(SandboxPolicyTest, FingerprintIsStableAcrossCallsOnSamePolicy) {
  auto policy = SandboxPolicy::default_hermetic();
  policy.allowed_env_vars.push_back("EXTRA");
  policy.allowed_paths.push_back({"//tools/clang", false});

  auto fp1 = policy.fingerprint();
  auto fp2 = policy.fingerprint();
  EXPECT_EQ(fp1, fp2);
}

TEST(SandboxPolicyTest, FingerprintIsStableRegardlessOfEnvVarInsertionOrder) {
  auto p1 = SandboxPolicy::default_hermetic();
  p1.allowed_env_vars.clear();
  p1.allowed_env_vars = {"B", "A", "C"};

  auto p2 = SandboxPolicy::default_hermetic();
  p2.allowed_env_vars.clear();
  p2.allowed_env_vars = {"C", "B", "A"};

  // Both policies have the same set of vars – fingerprints must match
  EXPECT_EQ(p1.fingerprint(), p2.fingerprint());
}

TEST(SandboxPolicyTest, EqualityOperatorMatchesFingerprintEquality) {
  auto p1 = SandboxPolicy::default_hermetic();
  auto p2 = SandboxPolicy::default_hermetic();
  EXPECT_TRUE(p1 == p2);

  p2.allowed_env_vars.push_back("EXTRA");
  EXPECT_TRUE(p1 != p2);
}

TEST(SandboxPolicyTest, ResourceLimitsAffectFingerprint) {
  auto p1 = SandboxPolicy::default_hermetic();
  auto p2 = SandboxPolicy::default_hermetic();
  p2.limits.timeout = std::chrono::seconds(300);
  EXPECT_NE(p1.fingerprint(), p2.fingerprint());
}

// ─────────────────────────────────────────────────────────────────────────────
// mix_policy_fingerprint (local_cache integration)
// ─────────────────────────────────────────────────────────────────────────────

TEST(MixPolicyFingerprintTest, DifferentPoliciesProduceDifferentMixedKeys) {
  const std::string label = "//examples/hello:app";
  const std::vector<uint8_t> key_bytes(label.begin(), label.end());
  auto base = compute_sha256(key_bytes);

  auto balanced = SandboxPolicy::default_hermetic();
  auto strict = SandboxPolicy::strict_hermetic();

  auto key_balanced = mix_policy_fingerprint(base, balanced.fingerprint());
  auto key_strict = mix_policy_fingerprint(base, strict.fingerprint());

  EXPECT_NE(key_balanced, key_strict);
}

TEST(MixPolicyFingerprintTest, SamePolicyProducesSameMixedKey) {
  const std::string label = "//examples/hello:app";
  const std::vector<uint8_t> key_bytes(label.begin(), label.end());
  auto base = compute_sha256(key_bytes);

  auto policy = SandboxPolicy::default_hermetic();
  auto key1 = mix_policy_fingerprint(base, policy.fingerprint());
  auto key2 = mix_policy_fingerprint(base, policy.fingerprint());
  EXPECT_EQ(key1, key2);
}

} // namespace horcrux::core::test
