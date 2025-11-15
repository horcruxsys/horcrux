# Horcrux x86-64 Baseline Toolchain
# Most portable - runs on any x86-64 CPU since 2003
# Compatible with: All modern CPUs

set(CMAKE_SYSTEM_NAME Linux)

# Baseline x86-64 architecture
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=x86-64 -mtune=generic -ffast-math -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -march=x86-64 -mtune=generic")

# Enable LTO for production builds
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# Standard warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-missing-field-initializers")

message(STATUS "Using x86-64 BASELINE toolchain: maximum compatibility")