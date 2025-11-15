# Horcrux CI Toolchain
# Safe + portable for GitHub Actions runners
# Prevents "illegal instruction" errors on CI

set(CMAKE_SYSTEM_NAME Linux)

# Safe baseline arch for GitHub runners (Xeon with no AVX2 support)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=x86-64 -mtune=generic -ffast-math -DNDEBUG")

# Disable LTO for CI (prevents illegal instruction)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# Set warnings - safe for CI with third-party dependencies
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-missing-field-initializers")

# Debug configuration for CI
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -march=x86-64 -mtune=generic")

message(STATUS "Using CI toolchain: portable x86-64 baseline, no LTO")