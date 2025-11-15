// Horcrux - Local Cache Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <chrono>
#include <filesystem>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/local_cache.h"

namespace horcrux::core::test {

// Helper function to create a temporary directory for testing
class TempDir {
public:
  TempDir() {
    path_ = std::filesystem::temp_directory_path() / "horcrux_cache_test";
    std::filesystem::create_directories(path_);
  }

  ~TempDir() {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }

  auto path() const -> const std::filesystem::path& {
    return path_;
  }

private:
  std::filesystem::path path_;
};

// Test SHA-256 hash computation
TEST(SHA256Test, ComputesCorrectHashForEmptyInput) {
  std::vector<uint8_t> data;
  auto hash = compute_sha256(data);

  // Expected SHA-256 of empty string
  Hash expected = {0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4,
                   0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b,
                   0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55};

  EXPECT_EQ(hash, expected);
}

TEST(SHA256Test, ComputesCorrectHashForKnownInput) {
  std::string data_str = "abc";
  std::vector<uint8_t> data(data_str.begin(), data_str.end());
  auto hash = compute_sha256(data);

  // Expected SHA-256 of "abc"
  Hash expected = {0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40,
                   0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17,
                   0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};

  EXPECT_EQ(hash, expected);
}

TEST(SHA256Test, HashIsDeterministic) {
  std::vector<uint8_t> data = {1, 2, 3, 4, 5};
  auto hash1 = compute_sha256(data);
  auto hash2 = compute_sha256(data);

  EXPECT_EQ(hash1, hash2);
}

TEST(SHA256Test, DifferentInputsProduceDifferentHashes) {
  std::vector<uint8_t> data1 = {1, 2, 3, 4, 5};
  std::vector<uint8_t> data2 = {1, 2, 3, 4, 6};

  auto hash1 = compute_sha256(data1);
  auto hash2 = compute_sha256(data2);

  EXPECT_NE(hash1, hash2);
}

// Test hash to string conversion
TEST(HashToStringTest, ConvertsHashToHexString) {
  Hash hash = {};
  for (size_t i = 0; i < hash.size(); ++i) {
    hash[i] = static_cast<uint8_t>(i);
  }

  auto str = hash_to_string(hash);

  EXPECT_EQ(str.length(), 64); // 32 bytes * 2 hex chars per byte
  EXPECT_EQ(str.substr(0, 2), "00");
  EXPECT_EQ(str.substr(2, 2), "01");
}

// Test LocalCache creation
TEST(LocalCacheTest, CreateSucceedsWithValidDirectory) {
  TempDir temp_dir;
  auto cache_result = LocalCache::create(temp_dir.path());

  ASSERT_TRUE(cache_result.has_value());
}

TEST(LocalCacheTest, CreateCreatesDirectoryIfNotExists) {
  TempDir temp_dir;
  auto cache_dir = temp_dir.path() / "new_cache";

  EXPECT_FALSE(std::filesystem::exists(cache_dir));

  auto cache_result = LocalCache::create(cache_dir);

  ASSERT_TRUE(cache_result.has_value());
  EXPECT_TRUE(std::filesystem::exists(cache_dir));
}

// Test cache store and lookup
TEST(LocalCacheTest, StoreAndLookupSucceed) {
  TempDir temp_dir;
  auto cache = LocalCache::create(temp_dir.path()).value();

  // Create test data
  std::vector<uint8_t> content = {1, 2, 3, 4, 5};
  auto hash = compute_sha256(content);

  Artifact artifact{.content = content,
                    .timestamp = std::chrono::system_clock::now().time_since_epoch().count()};

  // Store artifact
  auto store_result = cache.store(hash, artifact);
  ASSERT_TRUE(store_result.has_value());

  // Lookup artifact
  auto lookup_result = cache.lookup(hash);
  ASSERT_TRUE(lookup_result.has_value());
  EXPECT_EQ(lookup_result->content, content);
  EXPECT_EQ(lookup_result->timestamp, artifact.timestamp);
}

TEST(LocalCacheTest, LookupReturnsNulloptForMissingKey) {
  TempDir temp_dir;
  auto cache = LocalCache::create(temp_dir.path()).value();

  Hash missing_hash = {};
  auto lookup_result = cache.lookup(missing_hash);

  EXPECT_FALSE(lookup_result.has_value());
}

TEST(LocalCacheTest, ContainsReturnsTrueForStoredArtifact) {
  TempDir temp_dir;
  auto cache = LocalCache::create(temp_dir.path()).value();

  std::vector<uint8_t> content = {1, 2, 3};
  auto hash = compute_sha256(content);

  Artifact artifact{.content = content, .timestamp = 12345};
  cache.store(hash, artifact);

  EXPECT_TRUE(cache.contains(hash));
}

TEST(LocalCacheTest, ContainsReturnsFalseForMissingArtifact) {
  TempDir temp_dir;
  auto cache = LocalCache::create(temp_dir.path()).value();

  Hash missing_hash = {};
  EXPECT_FALSE(cache.contains(missing_hash));
}

// Test cache determinism
TEST(LocalCacheTest, SameHashAlwaysReturnsSameArtifact) {
  TempDir temp_dir;
  auto cache = LocalCache::create(temp_dir.path()).value();

  std::vector<uint8_t> content = {10, 20, 30};
  auto hash = compute_sha256(content);

  Artifact artifact{.content = content, .timestamp = 99999};
  cache.store(hash, artifact);

  // Lookup multiple times
  for (int i = 0; i < 10; ++i) {
    auto lookup_result = cache.lookup(hash);
    ASSERT_TRUE(lookup_result.has_value());
    EXPECT_EQ(lookup_result->content, content);
    EXPECT_EQ(lookup_result->timestamp, artifact.timestamp);
  }
}

// Test cache persistence
TEST(LocalCacheTest, ArtifactPersistsAcrossCacheInstances) {
  TempDir temp_dir;

  std::vector<uint8_t> content = {42, 43, 44};
  auto hash = compute_sha256(content);
  Artifact artifact{.content = content, .timestamp = 77777};

  // Store in first cache instance
  {
    auto cache = LocalCache::create(temp_dir.path()).value();
    cache.store(hash, artifact);
  }

  // Lookup in second cache instance
  {
    auto cache = LocalCache::create(temp_dir.path()).value();
    auto lookup_result = cache.lookup(hash);
    ASSERT_TRUE(lookup_result.has_value());
    EXPECT_EQ(lookup_result->content, content);
    EXPECT_EQ(lookup_result->timestamp, artifact.timestamp);
  }
}

// Test cache size
TEST(LocalCacheTest, SizeReturnsNumberOfArtifacts) {
  TempDir temp_dir;
  auto cache = LocalCache::create(temp_dir.path()).value();

  EXPECT_EQ(cache.size(), 0);

  // Add artifacts
  for (int i = 0; i < 5; ++i) {
    std::vector<uint8_t> content = {static_cast<uint8_t>(i)};
    auto hash = compute_sha256(content);
    Artifact artifact{.content = content, .timestamp = i};
    cache.store(hash, artifact);
  }

  EXPECT_EQ(cache.size(), 5);
}

// Test cache clear
TEST(LocalCacheTest, ClearRemovesAllArtifacts) {
  TempDir temp_dir;
  auto cache = LocalCache::create(temp_dir.path()).value();

  // Add artifacts
  std::vector<Hash> hashes;
  for (int i = 0; i < 3; ++i) {
    std::vector<uint8_t> content = {static_cast<uint8_t>(i)};
    auto hash = compute_sha256(content);
    hashes.push_back(hash);
    Artifact artifact{.content = content, .timestamp = i};
    cache.store(hash, artifact);
  }

  EXPECT_EQ(cache.size(), 3);

  // Clear cache
  auto clear_result = cache.clear();
  ASSERT_TRUE(clear_result.has_value());

  EXPECT_EQ(cache.size(), 0);

  // Verify artifacts are gone
  for (const auto& hash : hashes) {
    EXPECT_FALSE(cache.contains(hash));
  }
}

// Test with large content
TEST(LocalCacheTest, HandlesLargeContent) {
  TempDir temp_dir;
  auto cache = LocalCache::create(temp_dir.path()).value();

  // Create 1MB content
  std::vector<uint8_t> content(1024 * 1024);
  for (size_t i = 0; i < content.size(); ++i) {
    content[i] = static_cast<uint8_t>(i % 256);
  }

  auto hash = compute_sha256(content);
  Artifact artifact{.content = content, .timestamp = 12345};

  // Store and lookup
  auto store_result = cache.store(hash, artifact);
  ASSERT_TRUE(store_result.has_value());

  auto lookup_result = cache.lookup(hash);
  ASSERT_TRUE(lookup_result.has_value());
  EXPECT_EQ(lookup_result->content.size(), content.size());
  EXPECT_EQ(lookup_result->content, content);
}

// Test multiple artifacts with hash collisions prevention
TEST(LocalCacheTest, StoresMultipleDistinctArtifacts) {
  TempDir temp_dir;
  auto cache = LocalCache::create(temp_dir.path()).value();

  const int num_artifacts = 100;
  std::vector<Hash> hashes;
  std::vector<Artifact> artifacts;

  // Store multiple artifacts
  for (int i = 0; i < num_artifacts; ++i) {
    std::vector<uint8_t> content = {static_cast<uint8_t>(i), static_cast<uint8_t>(i >> 8)};
    auto hash = compute_sha256(content);
    Artifact artifact{.content = content, .timestamp = i};

    cache.store(hash, artifact);
    hashes.push_back(hash);
    artifacts.push_back(artifact);
  }

  // Verify all can be retrieved
  for (int i = 0; i < num_artifacts; ++i) {
    auto lookup_result = cache.lookup(hashes[i]);
    ASSERT_TRUE(lookup_result.has_value());
    EXPECT_EQ(lookup_result->content, artifacts[i].content);
    EXPECT_EQ(lookup_result->timestamp, artifacts[i].timestamp);
  }
}

} // namespace horcrux::core::test
