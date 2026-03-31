# Benchmark Methodology — Horcrux v2026.0401.0

This document describes the Horcrux Benchmark Battery (HBB): the workload
definitions, reference hardware profile, measurement methodology, and baseline
comparisons used to evaluate build-system performance.

## Goals

1. Provide reproducible, verifiable performance numbers.
2. Track cold build, warm build, and incremental rebuild times across releases.
3. Support fair comparison against selected baseline build systems.
4. Enforce a regression threshold policy in CI.

---

## Reference Hardware Profile

All official benchmark results are produced on the following configuration:

| Component | Specification |
|-----------|--------------|
| CPU | AMD Ryzen 9 7950X (16C/32T, 4.5 GHz base) |
| RAM | 64 GB DDR5-5600 ECC |
| Storage | Samsung 990 Pro 2 TB NVMe SSD |
| OS | Ubuntu 24.04 LTS (kernel 6.8) |
| Compiler | GCC 14.1 `-O2 -DNDEBUG` |
| CMake | 3.29.3 |
| Ninja | 1.11.1 |

> **Reproducibility note:** Raw benchmark data files are stored in
> `benchmarks/results/` and tagged with the Horcrux version and hardware
> fingerprint. Community results on different hardware are welcome but must
> include the full hardware profile.

---

## Workload Definitions

### WL-1: Small Graph — Cold Build

- **Description:** Build a synthetic C++ project with 10 libraries and 1
  binary (≈ 50 source files).
- **Metric:** Wall-clock time from empty cache to successful binary.
- **Baseline targets:** CMake/Ninja, Bazel 7.x.

### WL-2: Medium Graph — Cold Build

- **Description:** Build a synthetic C++ project with 100 libraries and 5
  binaries (≈ 500 source files).
- **Metric:** Wall-clock time from empty cache to all binaries.
- **Baseline targets:** CMake/Ninja, Bazel 7.x.

### WL-3: Large Graph — Cold Build

- **Description:** Build a synthetic polyglot project with 500 targets spanning
  C++, Rust, Python, and Java (≈ 2 000 source files).
- **Metric:** Wall-clock time from empty cache to all binaries.
- **Baseline targets:** CMake/Ninja (C++ subset only), Bazel 7.x.

### WL-4: Warm Build (Full Cache Hit)

- **Description:** Run `horcrux build //...` a second time on WL-2 after a
  successful cold build. All outputs should be served from cache.
- **Metric:** Wall-clock time; should be < 500 ms.
- **Baseline:** Bazel warm build.

### WL-5: Incremental Rebuild (Single File Change)

- **Description:** Change one leaf source file in WL-2 and rebuild.
- **Metric:** Wall-clock time; measures incremental invalidation accuracy.
- **Baseline:** CMake/Ninja, Bazel.

### WL-6: Dependency Resolution — Microbenchmark

- **Description:** Resolve transitive dependencies for a graph of 10 000 nodes
  using the `dependency_resolution_bench` benchmark.
- **Metric:** Mean latency per resolution cycle (nanoseconds).
- **Tool:** Google Benchmark (`benchmarks/dependency_resolution_bench.cpp`).

### WL-7: Cache Lookup — Microbenchmark

- **Description:** Perform 100 000 cache lookups with a warm cache of 10 000
  entries using `cache_lookup_bench`.
- **Metric:** Mean latency per lookup (nanoseconds).
- **Tool:** Google Benchmark (`benchmarks/cache_lookup_bench.cpp`).

### WL-8: Task Scheduling — Microbenchmark

- **Description:** Schedule 100 000 tasks through the priority queue using
  `task_scheduling_bench`.
- **Metric:** Mean scheduling latency (nanoseconds).
- **Tool:** Google Benchmark (`benchmarks/task_scheduling_bench.cpp`).

---

## Measurement Procedure

### End-to-End Benchmarks (WL-1 to WL-5)

```bash
# 1. Drop OS page cache to simulate cold storage
sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'

# 2. Remove Horcrux cache
horcrux clean --cache

# 3. Run the benchmark (5 iterations, take the median)
for i in 1 2 3 4 5; do
    /usr/bin/time -f "%e" horcrux build //... 2>&1 | tail -1
done

# 4. For warm-cache benchmarks: skip step 1 and 2, run immediately after cold build
```

### Microbenchmarks (WL-6 to WL-8)

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DHORCRUX_BUILD_BENCHMARKS=ON -G Ninja
cmake --build . -- -j$(nproc)

# Run with Google Benchmark JSON output
./benchmarks/dependency_resolution_bench --benchmark_format=json \
    --benchmark_out=benchmarks/results/v2026.0401.0/dependency_resolution.json

./benchmarks/cache_lookup_bench --benchmark_format=json \
    --benchmark_out=benchmarks/results/v2026.0401.0/cache_lookup.json

./benchmarks/task_scheduling_bench --benchmark_format=json \
    --benchmark_out=benchmarks/results/v2026.0401.0/task_scheduling.json
```

### Automated CI Collection

Benchmarks run automatically on every merge to `main` via
`.github/workflows/benchmarks.yml`. Results are published as workflow artifacts
and compared against the previous release using the regression threshold policy
(see below).

---

## Baseline Comparisons

### Methodology Notes

- Baseline results are measured on the **same reference hardware** using the
  latest stable release of each tool.
- Only workloads where the baseline tool has native support are included (e.g.,
  CMake is not compared on Rust-only workloads).
- Build outputs are validated to be equivalent (same binaries, same symbols)
  before any timing comparison is recorded.

### Summary — v2026.0401.0

| Workload | Horcrux | CMake/Ninja | Bazel 7.x | Notes |
|----------|---------|-------------|-----------|-------|
| WL-1 cold (small C++) | baseline | +12% | +8% | All systems comparable at small scale |
| WL-2 cold (medium C++) | baseline | +18% | +5% | Horcrux parallel scheduler advantage |
| WL-3 cold (large polyglot) | baseline | N/A (C++ only) | +22% | Bazel measured on C++ subset |
| WL-4 warm (full cache) | baseline | +450% | +30% | Horcrux cache lookup significantly faster |
| WL-5 incremental | baseline | +35% | +10% | Horcrux avoids over-invalidation |

> **Note:** The numbers in the table above are illustrative targets for the
> v2026.0401.0 release. Raw JSON results are in `benchmarks/results/v2026.0401.0/`.
> Community reproductions are encouraged — see the measurement procedure above.

---

## Regression Threshold Policy

A benchmark regression is defined as a **> 10% degradation** in median wall
time or mean latency relative to the previous release on the reference hardware.

CI enforces this policy automatically:

1. Benchmarks run on every `main` merge and on release candidates.
2. Results are compared against the `benchmarks/results/baseline/` snapshot.
3. If any workload degrades by > 10%, the CI job fails with a regression report.
4. A deliberate regression (e.g., accepting a trade-off for correctness) must
   be documented in the PR and the baseline snapshot updated explicitly.

To update the baseline snapshot after an intentional change:

```bash
cp benchmarks/results/v2026.0401.0/*.json benchmarks/results/baseline/
git add benchmarks/results/baseline/
git commit -m "benchmarks: update baseline to v2026.0401.0"
```

---

## Related Documents

- [CI Documentation](ci.md)
- [Release Process](release-process.md)
- [Benchmarks README](../benchmarks/README.md)
