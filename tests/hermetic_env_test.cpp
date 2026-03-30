// Horcrux - HermeticEnv Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <string>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/hermetic_env.h"
#include "../src/core/sandbox_policy.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build HermeticEnv from an explicit map
// ─────────────────────────────────────────────────────────────────────────────

static auto make_env(const SandboxPolicy& policy,
                     const std::unordered_map<std::string, std::string>& host_env)
    -> HermeticEnv {
  return HermeticEnv::build_from_map(policy, host_env, "/build/output");
}

// ─────────────────────────────────────────────────────────────────────────────
// Normalization overrides
// ─────────────────────────────────────────────────────────────────────────────

TEST(HermeticEnvTest, NormalizesLocale) {
  auto policy = SandboxPolicy::default_hermetic();
  std::unordered_map<std::string, std::string> host = {
      {"LANG", "ja_JP.UTF-8"}, {"PATH", "/usr/bin"}};
  auto env = make_env(policy, host);

  // Normalization must override the host locale
  EXPECT_EQ(env.get("LANG").value_or(""), std::string(HermeticEnv::kNormalizedLocale));
  EXPECT_EQ(env.get("LC_ALL").value_or(""), std::string(HermeticEnv::kNormalizedLocale));
}

TEST(HermeticEnvTest, NormalizesTimezone) {
  auto policy = SandboxPolicy::default_hermetic();
  std::unordered_map<std::string, std::string> host = {{"TZ", "America/New_York"}};
  auto env = make_env(policy, host);
  EXPECT_EQ(env.get("TZ").value_or(""), std::string(HermeticEnv::kNormalizedTimezone));
}

TEST(HermeticEnvTest, NormalizesTmpDir) {
  auto policy = SandboxPolicy::default_hermetic();
  std::unordered_map<std::string, std::string> host = {{"TMPDIR", "/home/user/tmp"}};
  auto env = make_env(policy, host);
  EXPECT_EQ(env.get("TMPDIR").value_or(""), std::string(HermeticEnv::kNormalizedTmpDir));
  EXPECT_EQ(env.get("TMP").value_or(""), std::string(HermeticEnv::kNormalizedTmpDir));
  EXPECT_EQ(env.get("TEMP").value_or(""), std::string(HermeticEnv::kNormalizedTmpDir));
}

TEST(HermeticEnvTest, InjectsOutputRoot) {
  auto policy = SandboxPolicy::default_hermetic();
  std::unordered_map<std::string, std::string> host;
  auto env = HermeticEnv::build_from_map(policy, host, "/my/output/root");
  EXPECT_EQ(env.get("HORCRUX_OUTPUT_ROOT").value_or(""), "/my/output/root");
}

// ─────────────────────────────────────────────────────────────────────────────
// Env-var filtering
// ─────────────────────────────────────────────────────────────────────────────

TEST(HermeticEnvTest, StripsVarsNotInAllowlist) {
  auto policy = SandboxPolicy::default_hermetic();
  std::unordered_map<std::string, std::string> host = {
      {"PATH", "/usr/bin"}, {"SECRET_TOKEN", "s3cr3t"}, {"GOPATH", "/go"}};
  auto env = make_env(policy, host);

  // SECRET_TOKEN and GOPATH are not in the allowlist – must be absent
  EXPECT_FALSE(env.get("SECRET_TOKEN").has_value());
  EXPECT_FALSE(env.get("GOPATH").has_value());
}

TEST(HermeticEnvTest, KeepsAllowedVarsFromHost) {
  auto policy = SandboxPolicy::default_hermetic();
  std::unordered_map<std::string, std::string> host = {{"HOME", "/home/builder"},
                                                        {"PATH", "/usr/local/bin:/usr/bin"}};
  auto env = make_env(policy, host);
  EXPECT_EQ(env.get("HOME").value_or(""), "/home/builder");
  EXPECT_EQ(env.get("PATH").value_or(""), "/usr/local/bin:/usr/bin");
}

TEST(HermeticEnvTest, OffModePassesThroughAllHostVars) {
  auto policy = SandboxPolicy::off();
  std::unordered_map<std::string, std::string> host = {
      {"CUSTOM_VAR", "hello"}, {"ANOTHER", "world"}};
  auto env = make_env(policy, host);

  // In Off mode, all host vars pass through (normalizations still apply)
  EXPECT_EQ(env.get("CUSTOM_VAR").value_or(""), "hello");
  EXPECT_EQ(env.get("ANOTHER").value_or(""), "world");
}

// ─────────────────────────────────────────────────────────────────────────────
// Sorted pairs
// ─────────────────────────────────────────────────────────────────────────────

TEST(HermeticEnvTest, AsSortedPairsIsSortedByKey) {
  auto policy = SandboxPolicy::default_hermetic();
  std::unordered_map<std::string, std::string> host = {
      {"HOME", "/home/user"}, {"PATH", "/usr/bin"}};
  auto env = make_env(policy, host);

  const auto& pairs = env.as_sorted_pairs();
  for (size_t i = 1; i < pairs.size(); ++i) {
    EXPECT_LT(pairs[i - 1].first, pairs[i].first)
        << "env pairs not sorted at index " << i;
  }
}

TEST(HermeticEnvTest, GetReturnsNulloptForMissingKey) {
  auto policy = SandboxPolicy::default_hermetic();
  std::unordered_map<std::string, std::string> host;
  auto env = make_env(policy, host);
  EXPECT_FALSE(env.get("NONEXISTENT_VAR").has_value());
}

TEST(HermeticEnvTest, SizeMatchesNumberOfPairs) {
  auto policy = SandboxPolicy::default_hermetic();
  std::unordered_map<std::string, std::string> host = {
      {"HOME", "/home/user"}, {"PATH", "/usr/bin"}};
  auto env = make_env(policy, host);
  EXPECT_EQ(env.size(), env.as_sorted_pairs().size());
}

// ─────────────────────────────────────────────────────────────────────────────
// Determinism: same inputs → same output
// ─────────────────────────────────────────────────────────────────────────────

TEST(HermeticEnvTest, SameInputsProduceSameOutput) {
  auto policy = SandboxPolicy::default_hermetic();
  std::unordered_map<std::string, std::string> host = {
      {"HOME", "/home/user"}, {"PATH", "/usr/bin"}, {"SECRET", "dropped"}};

  auto env1 = make_env(policy, host);
  auto env2 = make_env(policy, host);

  EXPECT_EQ(env1.as_sorted_pairs(), env2.as_sorted_pairs());
}

// ─────────────────────────────────────────────────────────────────────────────
// Canonical input ordering helpers
// ─────────────────────────────────────────────────────────────────────────────

TEST(HermeticEnvTest, SortInputsDedupAndSorts) {
  std::vector<std::string> inputs = {"c.cpp", "a.cpp", "b.cpp", "a.cpp"};
  auto sorted = HermeticEnv::sort_inputs(inputs);
  ASSERT_EQ(sorted.size(), 3u);
  EXPECT_EQ(sorted[0], "a.cpp");
  EXPECT_EQ(sorted[1], "b.cpp");
  EXPECT_EQ(sorted[2], "c.cpp");
}

TEST(HermeticEnvTest, SortInputsRemovesTrailingSlash) {
  std::vector<std::string> inputs = {"src/foo/", "src/bar"};
  auto sorted = HermeticEnv::sort_inputs(inputs);
  ASSERT_EQ(sorted.size(), 2u);
  EXPECT_EQ(sorted[0], "src/bar");
  EXPECT_EQ(sorted[1], "src/foo");
}

TEST(HermeticEnvTest, SortDepsDedupAndSorts) {
  std::vector<std::string> deps = {"//pkg:b", "//pkg:a", "//pkg:b"};
  auto sorted = HermeticEnv::sort_deps(deps);
  ASSERT_EQ(sorted.size(), 2u);
  EXPECT_EQ(sorted[0], "//pkg:a");
  EXPECT_EQ(sorted[1], "//pkg:b");
}

TEST(HermeticEnvTest, SortIncludesDeduplicates) {
  std::vector<std::string> incs = {"/usr/include", "/usr/local/include", "/usr/include"};
  auto sorted = HermeticEnv::sort_includes(incs);
  ASSERT_EQ(sorted.size(), 2u);
}

TEST(HermeticEnvTest, SortInputsIsStable) {
  // Running sort_inputs twice on the same data must give the same result
  std::vector<std::string> inputs = {"z.cpp", "a.cpp", "m.cpp"};
  auto s1 = HermeticEnv::sort_inputs(inputs);
  auto s2 = HermeticEnv::sort_inputs(inputs);
  EXPECT_EQ(s1, s2);
}

} // namespace horcrux::core::test
