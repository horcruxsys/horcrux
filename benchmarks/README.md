# Horcrux Benchmarks

This directory contains performance benchmarks for the Horcrux build system using Google Benchmark.

## Overview

The benchmarks measure critical performance metrics for Horcrux's core operations:

1. **Dependency Resolution** (`dependency_resolution_bench`) - Measures the time to resolve target dependencies in various graph structures
2. **Cache Lookup** (`cache_lookup_bench`) - Measures cache lookup latency with different cache sizes and access patterns
3. **Task Scheduling** (`task_scheduling_bench`) - Measures overhead of task scheduling operations

## Building Benchmarks

Benchmarks are built automatically with the main project:

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build .
```

The benchmark executables will be in `build/benchmarks/`.

## Running Benchmarks

### Run Individual Benchmarks

```bash
# Run dependency resolution benchmarks
./build/benchmarks/dependency_resolution_bench

# Run cache lookup benchmarks
./build/benchmarks/cache_lookup_bench

# Run task scheduling benchmarks
./build/benchmarks/task_scheduling_bench
```

### Run All Benchmarks

```bash
cd build
cmake --build . --target run_benchmarks
```

This will run all benchmarks and generate JSON result files in the build directory.

### Benchmark Options

Google Benchmark provides several command-line options:

```bash
# Run benchmarks with specific filter
./build/benchmarks/dependency_resolution_bench --benchmark_filter=Small

# Run benchmarks with custom iterations
./build/benchmarks/cache_lookup_bench --benchmark_min_time=5.0

# Output results in JSON format
./build/benchmarks/task_scheduling_bench --benchmark_format=json --benchmark_out=results.json

# Show help
./build/benchmarks/dependency_resolution_bench --help
```

## Benchmark Metrics

### Dependency Resolution Benchmarks

- **Small Graph**: 10 targets, 2 dependencies per target (average)
- **Medium Graph**: 100 targets, 5 dependencies per target (average)
- **Large Graph**: 1000 targets, 10 dependencies per target (average)
- **Deep Chain**: Linear chain of 100 targets (worst-case depth)
- **Wide Tree**: One target depending on 100 leaf targets

**Expected Performance**: Resolution should be O(V + E) where V = vertices (targets) and E = edges (dependencies).

### Cache Lookup Benchmarks

- **Hit/Miss Scenarios**: Benchmarks both successful lookups (cache hits) and failed lookups (cache misses)
- **Cache Sizes**: Small (100), Medium (10,000), Large (1,000,000) entries
- **Access Patterns**: Random, sequential, mixed (90% hit rate)

**Expected Performance**: Lookups should be O(1) average case with hash table implementation.

### Task Scheduling Benchmarks

- **Add Tasks**: Measures time to add tasks to the scheduler
- **Get Next Task**: Measures time to retrieve the next task from the scheduler
- **Full Cycle**: Complete add + get cycle
- **Variable Priority**: Scheduling with different priority ranges
- **Incremental**: Interleaved adding and executing tasks
- **High Contention**: Simulated multi-threaded contention

**Expected Performance**: Priority queue operations should be O(log N) for insert/remove.

## Baseline Metrics

Baseline metrics are collected and tracked to monitor performance regressions. Results are stored in JSON format and can be compared across commits.

### Storing Baselines

```bash
# Run benchmarks and save baseline
./build/benchmarks/dependency_resolution_bench \
    --benchmark_format=json \
    --benchmark_out=baseline_dependency.json
```

### Comparing Against Baselines

```bash
# Compare current results with baseline
python3 tools/compare_benchmarks.py \
    baseline_dependency.json \
    current_dependency.json
```

(Note: Comparison tool will be added in future updates)

## CI Integration

Benchmarks run automatically in CI on pull requests to detect performance regressions. Results are posted as PR comments showing:

- Time per operation
- Throughput (operations/second)
- Comparison with baseline (% change)

See `.github/workflows/benchmarks.yml` for CI configuration.

## Adding New Benchmarks

To add a new benchmark:

1. Create a new `.cpp` file in this directory
2. Include `<benchmark/benchmark.h>`
3. Define benchmark functions using `BENCHMARK()` macro
4. Add the executable to `CMakeLists.txt`
5. Update this README with benchmark description

Example:

```cpp
#include <benchmark/benchmark.h>

static void BM_MyNewBenchmark(benchmark::State& state) {
    for (auto _ : state) {
        // Code to benchmark
        benchmark::DoNotOptimize(my_function());
    }
}

BENCHMARK(BM_MyNewBenchmark);
```

## Performance Guidelines

When writing benchmarks:

1. **Use `DoNotOptimize()`** to prevent compiler from optimizing away the code
2. **Use `state.PauseTiming()` / `state.ResumeTiming()`** to exclude setup/teardown from measurements
3. **Set appropriate complexity** with `->Complexity()` to verify algorithmic complexity
4. **Use fixed seeds** for random number generators to ensure reproducibility
5. **Run in Release mode** with optimizations enabled (`-O3`)

## Troubleshooting

### Benchmarks take too long

Reduce the minimum time or iterations:

```bash
./benchmark --benchmark_min_time=1.0
```

### Results are unstable

- Ensure system is idle during benchmarks
- Run multiple times and average results
- Use `--benchmark_repetitions=N` to repeat benchmarks

### Out of memory

Reduce the size of large benchmarks or increase system memory.

## Further Reading

- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [Horcrux Architecture Document](../docs/architecture.md)
- [Performance Goals](../docs/architecture.md#benchmark-philosophy)
