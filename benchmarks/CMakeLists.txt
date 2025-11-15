# Benchmarks CMakeLists.txt
# Benchmark targets for Horcrux

# Fetch Google Benchmark
include(FetchContent)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.4
    GIT_SHALLOW TRUE
)

# Configure benchmark options
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable gtest tests" FORCE)

# Make benchmark available
FetchContent_MakeAvailable(benchmark)

# Common benchmark settings
set(BENCHMARK_COMMON_FLAGS
    $<$<CXX_COMPILER_ID:GNU>:
        -std=c++23
        -Wall -Wextra
        -O3
    >
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:
        -std=c++20
        -Wall -Wextra
        -O3
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /O2
    >
)

# Dependency resolution benchmark
add_executable(dependency_resolution_bench
    dependency_resolution_bench.cpp
)

target_link_libraries(dependency_resolution_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
)

target_compile_options(dependency_resolution_bench PRIVATE ${BENCHMARK_COMMON_FLAGS})

# Cache lookup latency benchmark
add_executable(cache_lookup_bench
    cache_lookup_bench.cpp
)

target_link_libraries(cache_lookup_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
)

target_compile_options(cache_lookup_bench PRIVATE ${BENCHMARK_COMMON_FLAGS})

# Task scheduling overhead benchmark
add_executable(task_scheduling_bench
    task_scheduling_bench.cpp
)

target_link_libraries(task_scheduling_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
)

target_compile_options(task_scheduling_bench PRIVATE ${BENCHMARK_COMMON_FLAGS})

# Local cache performance benchmark
add_executable(local_cache_bench
    local_cache_bench.cpp
)

target_link_libraries(local_cache_bench
    PRIVATE
    horcrux_core
    benchmark::benchmark
    benchmark::benchmark_main
)

target_compile_options(local_cache_bench PRIVATE ${BENCHMARK_COMMON_FLAGS})

# Installation
install(TARGETS
    dependency_resolution_bench
    cache_lookup_bench
    task_scheduling_bench
    local_cache_bench
    RUNTIME DESTINATION bin/benchmarks
)

# Custom target to run all benchmarks
add_custom_target(run_benchmarks
    COMMAND dependency_resolution_bench --benchmark_format=json --benchmark_out=dependency_resolution_results.json
    COMMAND cache_lookup_bench --benchmark_format=json --benchmark_out=cache_lookup_results.json
    COMMAND task_scheduling_bench --benchmark_format=json --benchmark_out=task_scheduling_results.json
    COMMAND local_cache_bench --benchmark_format=json --benchmark_out=local_cache_results.json
    DEPENDS
        dependency_resolution_bench
        cache_lookup_bench
        task_scheduling_bench
        local_cache_bench
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all benchmarks and generating JSON results"
)
