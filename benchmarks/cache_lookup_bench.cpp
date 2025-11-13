// Horcrux - Cache Lookup Latency Benchmark
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>

#include <benchmark/benchmark.h>

namespace horcrux::bench {

// Mock hash type (simulating SHA-256)
using Hash = std::array<uint8_t, 32>;

// Mock artifact (simplified)
struct MockArtifact {
  std::string content;
  size_t size;
  int64_t timestamp;
};

// Simple hash function for benchmarking
auto generate_hash(int seed) -> Hash {
  Hash hash{};
  std::mt19937 rng(seed);
  for (auto& byte : hash) {
    byte = static_cast<uint8_t>(rng() % 256);
  }
  return hash;
}

// Hash function for std::array
struct HashArrayHasher {
  auto operator()(const Hash& hash) const -> size_t {
    size_t result = 0;
    for (size_t i = 0; i < 8; ++i) {
      result ^= static_cast<size_t>(hash[i]) << (i * 8);
    }
    return result;
  }
};

// Mock in-memory cache
class MockMemoryCache {
public:
  void insert(const Hash& hash, const MockArtifact& artifact) {
    cache_[hash] = artifact;
  }

  auto lookup(const Hash& hash) const -> std::optional<MockArtifact> {
    auto it = cache_.find(hash);
    if (it != cache_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  auto size() const -> size_t {
    return cache_.size();
  }

private:
  std::unordered_map<Hash, MockArtifact, HashArrayHasher> cache_;
};

// Generate mock cache with specified number of entries
auto generate_mock_cache(int num_entries) -> MockMemoryCache {
  MockMemoryCache cache;

  for (int i = 0; i < num_entries; ++i) {
    Hash hash = generate_hash(i);
    MockArtifact artifact{.content = "artifact_content_" + std::to_string(i),
                          .size = static_cast<size_t>(1024 * (i % 100 + 1)),
                          .timestamp = 1700000000 + i};
    cache.insert(hash, artifact);
  }

  return cache;
}

// Benchmark: Cache hit - small cache (100 entries)
static void BM_CacheLookup_Hit_Small(benchmark::State& state) {
  const auto cache = generate_mock_cache(100);
  const Hash lookup_hash = generate_hash(50); // Known entry

  for (auto _ : state) {
    auto result = cache.lookup(lookup_hash);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["CacheSize"] = 100;
}

// Benchmark: Cache hit - medium cache (10,000 entries)
static void BM_CacheLookup_Hit_Medium(benchmark::State& state) {
  const auto cache = generate_mock_cache(10000);
  const Hash lookup_hash = generate_hash(5000); // Known entry

  for (auto _ : state) {
    auto result = cache.lookup(lookup_hash);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["CacheSize"] = 10000;
}

// Benchmark: Cache hit - large cache (1,000,000 entries)
static void BM_CacheLookup_Hit_Large(benchmark::State& state) {
  const auto cache = generate_mock_cache(1000000);
  const Hash lookup_hash = generate_hash(500000); // Known entry

  for (auto _ : state) {
    auto result = cache.lookup(lookup_hash);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["CacheSize"] = 1000000;
}

// Benchmark: Cache miss - small cache (100 entries)
static void BM_CacheLookup_Miss_Small(benchmark::State& state) {
  const auto cache = generate_mock_cache(100);
  const Hash lookup_hash = generate_hash(999999); // Unknown entry

  for (auto _ : state) {
    auto result = cache.lookup(lookup_hash);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["CacheSize"] = 100;
}

// Benchmark: Cache miss - medium cache (10,000 entries)
static void BM_CacheLookup_Miss_Medium(benchmark::State& state) {
  const auto cache = generate_mock_cache(10000);
  const Hash lookup_hash = generate_hash(999999); // Unknown entry

  for (auto _ : state) {
    auto result = cache.lookup(lookup_hash);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["CacheSize"] = 10000;
}

// Benchmark: Cache miss - large cache (1,000,000 entries)
static void BM_CacheLookup_Miss_Large(benchmark::State& state) {
  const auto cache = generate_mock_cache(1000000);
  const Hash lookup_hash = generate_hash(999999); // Unknown entry

  for (auto _ : state) {
    auto result = cache.lookup(lookup_hash);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["CacheSize"] = 1000000;
}

// Benchmark: Random lookups with 90% hit rate
static void BM_CacheLookup_MixedHitRate(benchmark::State& state) {
  const int cache_size = 10000;
  const auto cache = generate_mock_cache(cache_size);

  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, 99);

  for (auto _ : state) {
    // 90% chance of cache hit
    int seed = (dist(rng) < 90) ? (rng() % cache_size) : 999999;
    Hash lookup_hash = generate_hash(seed);

    auto result = cache.lookup(lookup_hash);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["CacheSize"] = cache_size;
  state.counters["HitRate"] = 0.9;
}

// Benchmark: Sequential access pattern
static void BM_CacheLookup_Sequential(benchmark::State& state) {
  const int cache_size = 10000;
  const auto cache = generate_mock_cache(cache_size);

  int index = 0;
  for (auto _ : state) {
    Hash lookup_hash = generate_hash(index % cache_size);
    auto result = cache.lookup(lookup_hash);
    benchmark::DoNotOptimize(result);
    ++index;
  }

  state.SetItemsProcessed(state.iterations());
  state.counters["CacheSize"] = cache_size;
}

// Register benchmarks
BENCHMARK(BM_CacheLookup_Hit_Small);
BENCHMARK(BM_CacheLookup_Hit_Medium);
BENCHMARK(BM_CacheLookup_Hit_Large);
BENCHMARK(BM_CacheLookup_Miss_Small);
BENCHMARK(BM_CacheLookup_Miss_Medium);
BENCHMARK(BM_CacheLookup_Miss_Large);
BENCHMARK(BM_CacheLookup_MixedHitRate);
BENCHMARK(BM_CacheLookup_Sequential);

} // namespace horcrux::bench
