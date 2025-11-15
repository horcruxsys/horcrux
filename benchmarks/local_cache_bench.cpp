// Horcrux - Local Cache Performance Benchmark
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include "../../src/core/local_cache.h"

namespace horcrux::bench {

using namespace horcrux::core;

// Helper to generate random content
auto generate_random_content(size_t size, int seed) -> std::vector<uint8_t> {
  std::vector<uint8_t> content(size);
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> dist(0, 255);

  for (auto& byte : content) {
    byte = static_cast<uint8_t>(dist(rng));
  }

  return content;
}

// Helper to create temp directory
auto create_benchmark_cache_dir() -> std::filesystem::path {
  auto temp_path = std::filesystem::temp_directory_path() / "horcrux_bench";
  std::filesystem::create_directories(temp_path);
  return temp_path;
}

// Clean up benchmark cache
void cleanup_benchmark_cache(const std::filesystem::path& path) {
  std::error_code ec;
  std::filesystem::remove_all(path, ec);
}

// Benchmark: Store operation
static void BM_Cache_Store_SmallArtifact(benchmark::State& state) {
  auto cache_dir = create_benchmark_cache_dir();
  auto cache = LocalCache::create(cache_dir).value();

  const size_t artifact_size = 1024; // 1 KB
  int seed = 0;

  for (auto _ : state) {
    auto content = generate_random_content(artifact_size, seed++);
    auto hash = compute_sha256(content);
    Artifact artifact{.content = content,
                      .timestamp = std::chrono::system_clock::now().time_since_epoch().count()};

    auto result = cache.store(hash, artifact);
    benchmark::DoNotOptimize(result);
  }

  cleanup_benchmark_cache(cache_dir);

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * artifact_size);
}

static void BM_Cache_Store_MediumArtifact(benchmark::State& state) {
  auto cache_dir = create_benchmark_cache_dir();
  auto cache = LocalCache::create(cache_dir).value();

  const size_t artifact_size = 1024 * 100; // 100 KB
  int seed = 0;

  for (auto _ : state) {
    auto content = generate_random_content(artifact_size, seed++);
    auto hash = compute_sha256(content);
    Artifact artifact{.content = content,
                      .timestamp = std::chrono::system_clock::now().time_since_epoch().count()};

    auto result = cache.store(hash, artifact);
    benchmark::DoNotOptimize(result);
  }

  cleanup_benchmark_cache(cache_dir);

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * artifact_size);
}

// Benchmark: Lookup operation (cache hit)
static void BM_Cache_Lookup_Hit(benchmark::State& state) {
  auto cache_dir = create_benchmark_cache_dir();
  auto cache = LocalCache::create(cache_dir).value();

  // Pre-populate cache
  const int num_artifacts = 1000;
  std::vector<Hash> hashes;

  for (int i = 0; i < num_artifacts; ++i) {
    auto content = generate_random_content(1024, i);
    auto hash = compute_sha256(content);
    Artifact artifact{.content = content, .timestamp = i};
    cache.store(hash, artifact);
    hashes.push_back(hash);
  }

  int index = 0;
  for (auto _ : state) {
    auto result = cache.lookup(hashes[index % num_artifacts]);
    benchmark::DoNotOptimize(result);
    ++index;
  }

  cleanup_benchmark_cache(cache_dir);

  state.SetItemsProcessed(state.iterations());
  state.counters["CacheSize"] = num_artifacts;
}

// Benchmark: Lookup operation (cache miss)
static void BM_Cache_Lookup_Miss(benchmark::State& state) {
  auto cache_dir = create_benchmark_cache_dir();
  auto cache = LocalCache::create(cache_dir).value();

  // Pre-populate cache with some artifacts
  for (int i = 0; i < 100; ++i) {
    auto content = generate_random_content(1024, i);
    auto hash = compute_sha256(content);
    Artifact artifact{.content = content, .timestamp = i};
    cache.store(hash, artifact);
  }

  int seed = 10000; // Use different seed to ensure misses
  for (auto _ : state) {
    auto content = generate_random_content(1024, seed++);
    auto hash = compute_sha256(content);
    auto result = cache.lookup(hash);
    benchmark::DoNotOptimize(result);
  }

  cleanup_benchmark_cache(cache_dir);

  state.SetItemsProcessed(state.iterations());
}

// Benchmark: Mixed hit/miss pattern (90% hit rate)
static void BM_Cache_Lookup_MixedHitRate(benchmark::State& state) {
  auto cache_dir = create_benchmark_cache_dir();
  auto cache = LocalCache::create(cache_dir).value();

  // Pre-populate cache
  const int num_cached = 1000;
  std::vector<Hash> cached_hashes;

  for (int i = 0; i < num_cached; ++i) {
    auto content = generate_random_content(1024, i);
    auto hash = compute_sha256(content);
    Artifact artifact{.content = content, .timestamp = i};
    cache.store(hash, artifact);
    cached_hashes.push_back(hash);
  }

  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, 99);
  int miss_seed = 100000;

  for (auto _ : state) {
    Hash lookup_hash;
    if (dist(rng) < 90) {
      // 90% hit
      lookup_hash = cached_hashes[rng() % num_cached];
    } else {
      // 10% miss
      auto content = generate_random_content(1024, miss_seed++);
      lookup_hash = compute_sha256(content);
    }

    auto result = cache.lookup(lookup_hash);
    benchmark::DoNotOptimize(result);
  }

  cleanup_benchmark_cache(cache_dir);

  state.SetItemsProcessed(state.iterations());
  state.counters["HitRate"] = 0.9;
}

// Benchmark: Contains operation
static void BM_Cache_Contains(benchmark::State& state) {
  auto cache_dir = create_benchmark_cache_dir();
  auto cache = LocalCache::create(cache_dir).value();

  // Pre-populate cache
  const int num_artifacts = 1000;
  std::vector<Hash> hashes;

  for (int i = 0; i < num_artifacts; ++i) {
    auto content = generate_random_content(1024, i);
    auto hash = compute_sha256(content);
    Artifact artifact{.content = content, .timestamp = i};
    cache.store(hash, artifact);
    hashes.push_back(hash);
  }

  int index = 0;
  for (auto _ : state) {
    bool result = cache.contains(hashes[index % num_artifacts]);
    benchmark::DoNotOptimize(result);
    ++index;
  }

  cleanup_benchmark_cache(cache_dir);

  state.SetItemsProcessed(state.iterations());
}

// Benchmark: SHA-256 computation
static void BM_SHA256_Small(benchmark::State& state) {
  auto content = generate_random_content(1024, 42); // 1 KB

  for (auto _ : state) {
    auto hash = compute_sha256(content);
    benchmark::DoNotOptimize(hash);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * content.size());
}

static void BM_SHA256_Medium(benchmark::State& state) {
  auto content = generate_random_content(1024 * 100, 42); // 100 KB

  for (auto _ : state) {
    auto hash = compute_sha256(content);
    benchmark::DoNotOptimize(hash);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * content.size());
}

static void BM_SHA256_Large(benchmark::State& state) {
  auto content = generate_random_content(1024 * 1024, 42); // 1 MB

  for (auto _ : state) {
    auto hash = compute_sha256(content);
    benchmark::DoNotOptimize(hash);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * content.size());
}

// Benchmark: End-to-end (store + lookup)
static void BM_Cache_StoreAndLookup(benchmark::State& state) {
  auto cache_dir = create_benchmark_cache_dir();
  auto cache = LocalCache::create(cache_dir).value();

  int seed = 0;
  const size_t artifact_size = 1024;

  for (auto _ : state) {
    // Store
    auto content = generate_random_content(artifact_size, seed++);
    auto hash = compute_sha256(content);
    Artifact artifact{.content = content,
                      .timestamp = std::chrono::system_clock::now().time_since_epoch().count()};

    cache.store(hash, artifact);

    // Lookup
    auto result = cache.lookup(hash);
    benchmark::DoNotOptimize(result);
  }

  cleanup_benchmark_cache(cache_dir);

  state.SetItemsProcessed(state.iterations());
}

// Register benchmarks
BENCHMARK(BM_Cache_Store_SmallArtifact);
BENCHMARK(BM_Cache_Store_MediumArtifact);
BENCHMARK(BM_Cache_Lookup_Hit);
BENCHMARK(BM_Cache_Lookup_Miss);
BENCHMARK(BM_Cache_Lookup_MixedHitRate);
BENCHMARK(BM_Cache_Contains);
BENCHMARK(BM_SHA256_Small);
BENCHMARK(BM_SHA256_Medium);
BENCHMARK(BM_SHA256_Large);
BENCHMARK(BM_Cache_StoreAndLookup);

} // namespace horcrux::bench
