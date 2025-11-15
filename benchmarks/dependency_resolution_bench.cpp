// Horcrux - Dependency Resolution Benchmark
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <algorithm>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <benchmark/benchmark.h>

namespace horcrux::bench {

// Mock dependency graph structure for benchmarking
struct MockTarget {
  std::string name;
  std::vector<std::string> dependencies;
};

// Simple mock dependency resolver
class MockDependencyResolver {
public:
  explicit MockDependencyResolver(const std::unordered_map<std::string, MockTarget>& targets)
      : targets_(targets) {
  }

  auto resolve(const std::string& target_name) const -> std::vector<std::string> {
    std::vector<std::string> resolved;
    std::unordered_set<std::string> visited;
    resolve_recursive(target_name, resolved, visited);
    return resolved;
  }

private:
  void resolve_recursive(const std::string& target_name, std::vector<std::string>& resolved,
                         std::unordered_set<std::string>& visited) const {
    if (visited.contains(target_name)) {
      return;
    }

    visited.insert(target_name);

    auto it = targets_.find(target_name);
    if (it == targets_.end()) {
      return;
    }

    const auto& target = it->second;
    for (const auto& dep : target.dependencies) {
      resolve_recursive(dep, resolved, visited);
    }

    resolved.push_back(target_name);
  }

  const std::unordered_map<std::string, MockTarget>& targets_;
};

// Generate a mock dependency graph with controlled complexity
auto generate_mock_graph(int num_targets,
                         int avg_deps_per_target) -> std::unordered_map<std::string, MockTarget> {
  std::unordered_map<std::string, MockTarget> targets;
  std::mt19937 rng(42); // Fixed seed for reproducibility
  std::uniform_int_distribution<int> dep_count_dist(0, avg_deps_per_target * 2);

  // Create targets
  for (int i = 0; i < num_targets; ++i) {
    MockTarget target;
    target.name = "target_" + std::to_string(i);

    // Add dependencies to earlier targets to avoid cycles
    if (i > 0) {
      int num_deps = std::min(i, dep_count_dist(rng));
      std::uniform_int_distribution<int> target_dist(0, i - 1);

      for (int j = 0; j < num_deps; ++j) {
        int dep_idx = target_dist(rng);
        target.dependencies.push_back("target_" + std::to_string(dep_idx));
      }
    }

    targets[target.name] = std::move(target);
  }

  return targets;
}

// Benchmark: Small dependency graph (10 targets, 2 deps avg)
static void BM_DependencyResolution_Small(benchmark::State& state) {
  const auto targets = generate_mock_graph(10, 2);
  const MockDependencyResolver resolver(targets);

  for (auto _ : state) {
    auto resolved = resolver.resolve("target_9");
    benchmark::DoNotOptimize(resolved);
  }

  state.SetItemsProcessed(state.iterations());
}

// Benchmark: Medium dependency graph (100 targets, 5 deps avg)
static void BM_DependencyResolution_Medium(benchmark::State& state) {
  const auto targets = generate_mock_graph(100, 5);
  const MockDependencyResolver resolver(targets);

  for (auto _ : state) {
    auto resolved = resolver.resolve("target_99");
    benchmark::DoNotOptimize(resolved);
  }

  state.SetItemsProcessed(state.iterations());
}

// Benchmark: Large dependency graph (1000 targets, 10 deps avg)
static void BM_DependencyResolution_Large(benchmark::State& state) {
  const auto targets = generate_mock_graph(1000, 10);
  const MockDependencyResolver resolver(targets);

  for (auto _ : state) {
    auto resolved = resolver.resolve("target_999");
    benchmark::DoNotOptimize(resolved);
  }

  state.SetItemsProcessed(state.iterations());
}

// Benchmark: Deep dependency chain (linear chain of 100 targets)
static void BM_DependencyResolution_DeepChain(benchmark::State& state) {
  std::unordered_map<std::string, MockTarget> targets;

  // Create a linear dependency chain: target_99 -> target_98 -> ... -> target_0
  for (int i = 0; i < 100; ++i) {
    MockTarget target;
    target.name = "target_" + std::to_string(i);
    if (i > 0) {
      target.dependencies.push_back("target_" + std::to_string(i - 1));
    }
    targets[target.name] = std::move(target);
  }

  const MockDependencyResolver resolver(targets);

  for (auto _ : state) {
    auto resolved = resolver.resolve("target_99");
    benchmark::DoNotOptimize(resolved);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(100);
}

// Benchmark: Wide dependency tree (one target depends on many)
static void BM_DependencyResolution_WideTree(benchmark::State& state) {
  std::unordered_map<std::string, MockTarget> targets;

  // Create 100 leaf targets with no dependencies
  for (int i = 0; i < 100; ++i) {
    MockTarget target;
    target.name = "target_" + std::to_string(i);
    targets[target.name] = std::move(target);
  }

  // Create root target depending on all leaf targets
  MockTarget root;
  root.name = "root";
  for (int i = 0; i < 100; ++i) {
    root.dependencies.push_back("target_" + std::to_string(i));
  }
  targets[root.name] = std::move(root);

  const MockDependencyResolver resolver(targets);

  for (auto _ : state) {
    auto resolved = resolver.resolve("root");
    benchmark::DoNotOptimize(resolved);
  }

  state.SetItemsProcessed(state.iterations());
}

// Register benchmarks
BENCHMARK(BM_DependencyResolution_Small);
BENCHMARK(BM_DependencyResolution_Medium);
BENCHMARK(BM_DependencyResolution_Large);
BENCHMARK(BM_DependencyResolution_DeepChain)->Complexity();
BENCHMARK(BM_DependencyResolution_WideTree);

} // namespace horcrux::bench
