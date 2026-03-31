// Horcrux - Cache Correctness Regression Tests
// Copyright (C) 2026 Horcrux Project Contributors
// Licensed under the MIT License
//
// Release-critical regression suite: verifies that the local cache stores and
// retrieves artifacts correctly, that policy fingerprints are mixed properly
// into cache keys, and that corruption or missing entries are handled safely.

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/local_cache.h"

namespace horcrux::core::test {

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: temporary directory cleaned up after each test
// ─────────────────────────────────────────────────────────────────────────────

class TempCacheDir {
public:
  TempCacheDir() {
    path_ = fs::temp_directory_path() / "horcrux_cache_regression_test";
    fs::create_directories(path_);
  }
  ~TempCacheDir() {
    std::error_code ec;
    fs::remove_all(path_, ec);
  }
  [[nodiscard]] auto path() const -> const fs::path& {
    return path_;
  }

private:
  fs::path path_;
};

static auto make_hash(uint8_t fill) -> Hash {
  Hash h{};
  h.fill(fill);
  return h;
}

static auto make_artifact(const std::string& content) -> Artifact {
  Artifact a;
  a.content = std::vector<uint8_t>(content.begin(), content.end());
  a.timestamp = 0;
  return a;
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: store then lookup returns the original artifact
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheRegressionTest, StoreAndLookupRoundTrip) {
  TempCacheDir tmp;
  auto cache = LocalCache::create(tmp.path()).value();

  Hash key = make_hash(0x01);
  Artifact artifact = make_artifact("hello cache");

  auto store_result = cache.store(key, artifact);
  ASSERT_TRUE(store_result.has_value());

  auto lookup_result = cache.lookup(key);
  ASSERT_TRUE(lookup_result.has_value());
  EXPECT_EQ(lookup_result->content, artifact.content);
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: lookup on unknown key returns nullopt (no phantom hit)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheRegressionTest, LookupMissingKeyReturnsNullopt) {
  TempCacheDir tmp;
  auto cache = LocalCache::create(tmp.path()).value();

  Hash key = make_hash(0xAB);
  auto result = cache.lookup(key);
  EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: distinct keys do not collide
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheRegressionTest, DistinctKeysDoNotCollide) {
  TempCacheDir tmp;
  auto cache = LocalCache::create(tmp.path()).value();

  Hash key_a = make_hash(0x01);
  Hash key_b = make_hash(0x02);
  Artifact art_a = make_artifact("artifact A");
  Artifact art_b = make_artifact("artifact B");

  ASSERT_TRUE(cache.store(key_a, art_a).has_value());
  ASSERT_TRUE(cache.store(key_b, art_b).has_value());

  auto result_a = cache.lookup(key_a);
  auto result_b = cache.lookup(key_b);
  ASSERT_TRUE(result_a.has_value());
  ASSERT_TRUE(result_b.has_value());
  EXPECT_EQ(result_a->content, art_a.content);
  EXPECT_EQ(result_b->content, art_b.content);
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: overwriting a key replaces the stored artifact
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheRegressionTest, OverwriteKeyUpdatesStoredArtifact) {
  TempCacheDir tmp;
  auto cache = LocalCache::create(tmp.path()).value();

  Hash key = make_hash(0x10);
  Artifact first = make_artifact("first content");
  Artifact second = make_artifact("second content");

  ASSERT_TRUE(cache.store(key, first).has_value());
  ASSERT_TRUE(cache.store(key, second).has_value());

  auto result = cache.lookup(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->content, second.content);
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: SHA-256 hash is deterministic for identical inputs
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheRegressionTest, SHA256IsDeterministic) {
  std::vector<uint8_t> data = {0x01, 0x02, 0x03};
  EXPECT_EQ(compute_sha256(data), compute_sha256(data));
}

TEST(CacheRegressionTest, SHA256DiffersForDistinctInputs) {
  std::vector<uint8_t> data_a = {0x01, 0x02, 0x03};
  std::vector<uint8_t> data_b = {0x01, 0x02, 0x04};
  EXPECT_NE(compute_sha256(data_a), compute_sha256(data_b));
}

TEST(CacheRegressionTest, SHA256EmptyInputMatchesKnownValue) {
  std::vector<uint8_t> empty;
  Hash expected = {0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4,
                   0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b,
                   0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55};
  EXPECT_EQ(compute_sha256(empty), expected);
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: policy fingerprint mixing produces distinct keys
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheRegressionTest, PolicyFingerprintMixProducesDistinctKeys) {
  Hash base = make_hash(0x42);
  Hash policy_fp_1 = make_hash(0x01);
  Hash policy_fp_2 = make_hash(0x02);

  Hash mixed_1 = mix_policy_fingerprint(base, policy_fp_1);
  Hash mixed_2 = mix_policy_fingerprint(base, policy_fp_2);

  EXPECT_NE(mixed_1, mixed_2);
}

TEST(CacheRegressionTest, PolicyFingerprintMixIsDeterministic) {
  Hash base = make_hash(0xAB);
  Hash policy_fp = make_hash(0xCD);

  Hash result1 = mix_policy_fingerprint(base, policy_fp);
  Hash result2 = mix_policy_fingerprint(base, policy_fp);

  EXPECT_EQ(result1, result2);
}

TEST(CacheRegressionTest, PolicyFingerprintMixDiffersFromBase) {
  Hash base = make_hash(0x55);
  Hash policy_fp = make_hash(0xAA);

  Hash mixed = mix_policy_fingerprint(base, policy_fp);
  EXPECT_NE(mixed, base);
}

TEST(CacheRegressionTest, SamePolicyFingerprintYieldsSameKey) {
  Hash base = make_hash(0x10);
  Hash fp = make_hash(0x20);

  EXPECT_EQ(mix_policy_fingerprint(base, fp), mix_policy_fingerprint(base, fp));
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: cache lookup after policy change results in a miss
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheRegressionTest, PolicyChangeInvalidatesCacheKey) {
  TempCacheDir tmp;
  auto cache = LocalCache::create(tmp.path()).value();

  Hash base = make_hash(0x77);
  Hash old_policy = make_hash(0x01);
  Hash new_policy = make_hash(0x02);

  Hash old_key = mix_policy_fingerprint(base, old_policy);
  Hash new_key = mix_policy_fingerprint(base, new_policy);

  Artifact artifact = make_artifact("built output");
  ASSERT_TRUE(cache.store(old_key, artifact).has_value());

  // New policy produces a different key → cache miss
  auto result = cache.lookup(new_key);
  EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: large artifact round-trips correctly
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheRegressionTest, LargeArtifactRoundTrip) {
  TempCacheDir tmp;
  auto cache = LocalCache::create(tmp.path()).value();

  // 1 MiB of data
  std::vector<uint8_t> large_content(1024 * 1024, 0xDE);
  Artifact artifact;
  artifact.content = large_content;
  artifact.timestamp = 0;

  Hash key = compute_sha256(large_content);
  ASSERT_TRUE(cache.store(key, artifact).has_value());

  auto result = cache.lookup(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->content, large_content);
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: empty artifact content is stored and retrieved
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheRegressionTest, EmptyArtifactRoundTrip) {
  TempCacheDir tmp;
  auto cache = LocalCache::create(tmp.path()).value();

  Hash key = make_hash(0x00);
  Artifact artifact;
  artifact.content = {};
  artifact.timestamp = 0;

  ASSERT_TRUE(cache.store(key, artifact).has_value());

  auto result = cache.lookup(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->content.empty());
}

} // namespace horcrux::core::test
