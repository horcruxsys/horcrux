# Horcrux x86-64-v4 Toolchain
# Requires: AVX512F, AVX512BW, AVX512CD, AVX512DQ, AVX512VL
# Compatible with: Intel Skylake-X (2017+), AMD Zen4 (2022+)
# Good for: HPC workloads, latest server hardware

set(CMAKE_SYSTEM_NAME Linux)

# x86-64-v4 microarchitecture level
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=x86-64-v4 -mtune=generic -ffast-math -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -march=x86-64-v4 -mtune=generic")

# Enable LTO for production builds
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# Standard warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-missing-field-initializers")

message(STATUS "Using x86-64-V4 toolchain: AVX512 instruction set")