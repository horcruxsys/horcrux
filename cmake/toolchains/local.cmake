# Horcrux Local Development Toolchain
# Maximum performance for local workstation builds
# Uses native CPU optimizations and full LTO

set(CMAKE_SYSTEM_NAME Linux)

# Native tuning - optimized for your specific CPU
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -ffast-math -DNDEBUG")

# Full LTO for maximum performance
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# Aggressive warnings for development
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror=return-type")

# Debug configuration with native optimizations
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -march=native -mtune=native")

# Enable additional optimizations for local builds
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto=thin")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto=thin")
endif()

message(STATUS "Using LOCAL toolchain: native CPU tuning + full LTO")