// Horcrux - Task Scheduling Overhead Benchmark
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <algorithm>
#include <atomic>
#include <chrono>
#include <queue>
#include <random>
#include <thread>
#include <vector>

#include <benchmark/benchmark.h>

namespace horcrux::bench {

// Mock task representation
struct MockTask {
  int id;
  int priority;
  std::vector<int> dependencies;
  std::chrono::microseconds estimated_duration;

  auto operator<(const MockTask& other) const -> bool {
    // Higher priority values execute first
    return priority < other.priority;
  }
};

// Simple mock task scheduler
class MockTaskScheduler {
public:
  void add_task(MockTask task) {
    queue_.push(std::move(task));
  }

  auto get_next_task() -> std::optional<MockTask> {
    if (queue_.empty()) {
      return std::nullopt;
    }
    MockTask task = queue_.top();
    queue_.pop();
    return task;
  }

  auto size() const -> size_t {
    return queue_.size();
  }

private:
  std::priority_queue<MockTask> queue_;
};

// Generate mock tasks
auto generate_mock_tasks(int num_tasks, int max_priority) -> std::vector<MockTask> {
  std::vector<MockTask> tasks;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> priority_dist(1, max_priority);
  std::uniform_int_distribution<int> duration_dist(10, 1000);

  for (int i = 0; i < num_tasks; ++i) {
    MockTask task{.id = i,
                  .priority = priority_dist(rng),
                  .dependencies = {},
                  .estimated_duration = std::chrono::microseconds(duration_dist(rng))};
    tasks.push_back(task);
  }

  return tasks;
}

// Benchmark: Add tasks to scheduler
static void BM_TaskScheduling_AddTasks(benchmark::State& state) {
  const int num_tasks = static_cast<int>(state.range(0));
  const auto tasks = generate_mock_tasks(num_tasks, 10);

  for (auto _ : state) {
    MockTaskScheduler scheduler;
    for (const auto& task : tasks) {
      scheduler.add_task(task);
    }
    benchmark::DoNotOptimize(scheduler);
  }

  state.SetItemsProcessed(state.iterations() * num_tasks);
  state.SetComplexityN(num_tasks);
}

// Benchmark: Get next task from scheduler
static void BM_TaskScheduling_GetNextTask(benchmark::State& state) {
  const int num_tasks = static_cast<int>(state.range(0));
  const auto tasks = generate_mock_tasks(num_tasks, 10);

  for (auto _ : state) {
    state.PauseTiming();
    MockTaskScheduler scheduler;
    for (const auto& task : tasks) {
      scheduler.add_task(task);
    }
    state.ResumeTiming();

    while (auto task = scheduler.get_next_task()) {
      benchmark::DoNotOptimize(task);
    }
  }

  state.SetItemsProcessed(state.iterations() * num_tasks);
  state.SetComplexityN(num_tasks);
}

// Benchmark: Full scheduling cycle (add + get)
static void BM_TaskScheduling_FullCycle(benchmark::State& state) {
  const int num_tasks = static_cast<int>(state.range(0));
  const auto tasks = generate_mock_tasks(num_tasks, 10);

  for (auto _ : state) {
    MockTaskScheduler scheduler;

    // Add all tasks
    for (const auto& task : tasks) {
      scheduler.add_task(task);
    }

    // Get all tasks
    while (auto task = scheduler.get_next_task()) {
      benchmark::DoNotOptimize(task);
    }
  }

  state.SetItemsProcessed(state.iterations() * num_tasks);
  state.SetComplexityN(num_tasks);
}

// Benchmark: Scheduling with variable priority distribution
static void BM_TaskScheduling_VariablePriority(benchmark::State& state) {
  const int num_tasks = 1000;
  const int max_priority = static_cast<int>(state.range(0));
  const auto tasks = generate_mock_tasks(num_tasks, max_priority);

  for (auto _ : state) {
    MockTaskScheduler scheduler;

    for (const auto& task : tasks) {
      scheduler.add_task(task);
    }

    while (auto task = scheduler.get_next_task()) {
      benchmark::DoNotOptimize(task);
    }
  }

  state.SetItemsProcessed(state.iterations() * num_tasks);
  state.counters["MaxPriority"] = max_priority;
}

// Benchmark: Incremental scheduling (add and execute interleaved)
static void BM_TaskScheduling_Incremental(benchmark::State& state) {
  const int num_tasks = static_cast<int>(state.range(0));
  const auto tasks = generate_mock_tasks(num_tasks, 10);

  for (auto _ : state) {
    MockTaskScheduler scheduler;
    size_t task_index = 0;

    // Interleave adding and getting tasks
    while (task_index < tasks.size()) {
      // Add a batch of tasks
      const size_t batch_size = std::min<size_t>(10, tasks.size() - task_index);
      for (size_t i = 0; i < batch_size; ++i) {
        scheduler.add_task(tasks[task_index++]);
      }

      // Execute some tasks
      for (size_t i = 0; i < batch_size && scheduler.size() > 0; ++i) {
        auto task = scheduler.get_next_task();
        benchmark::DoNotOptimize(task);
      }
    }

    // Execute remaining tasks
    while (auto task = scheduler.get_next_task()) {
      benchmark::DoNotOptimize(task);
    }
  }

  state.SetItemsProcessed(state.iterations() * num_tasks);
  state.SetComplexityN(num_tasks);
}

// Benchmark: High-contention scenario (simulated with atomic operations)
static void BM_TaskScheduling_HighContention(benchmark::State& state) {
  const int num_tasks = 1000;
  const auto tasks = generate_mock_tasks(num_tasks, 10);

  std::atomic<int> counter{0};

  for (auto _ : state) {
    MockTaskScheduler scheduler;

    for (const auto& task : tasks) {
      scheduler.add_task(task);
    }

    while (auto task = scheduler.get_next_task()) {
      // Simulate atomic operation overhead
      counter.fetch_add(1, std::memory_order_relaxed);
      benchmark::DoNotOptimize(task);
    }
  }

  state.SetItemsProcessed(state.iterations() * num_tasks);
}

// Register benchmarks with range arguments
BENCHMARK(BM_TaskScheduling_AddTasks)->RangeMultiplier(10)->Range(10, 10000)->Complexity();

BENCHMARK(BM_TaskScheduling_GetNextTask)->RangeMultiplier(10)->Range(10, 10000)->Complexity();

BENCHMARK(BM_TaskScheduling_FullCycle)->RangeMultiplier(10)->Range(10, 10000)->Complexity();

BENCHMARK(BM_TaskScheduling_VariablePriority)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);

BENCHMARK(BM_TaskScheduling_Incremental)->RangeMultiplier(10)->Range(10, 10000)->Complexity();

BENCHMARK(BM_TaskScheduling_HighContention);

} // namespace horcrux::bench
