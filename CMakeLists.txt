cmake_minimum_required(VERSION 3.25)

# Project definition
project(Horcrux
    VERSION 0.1.0
    DESCRIPTION "The Next-Generation Universal Build System"
    LANGUAGES CXX
)

# Require C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler-specific flags for C++23 and optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Enable warnings and optimizations
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror=return-type
    )
    
    # Release build optimizations
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        add_compile_options(-O3)
    endif()
    
elseif(MSVC)
    # MSVC-specific flags
    add_compile_options(
        /W4
        /permissive-
        /std:c++latest
    )
    
    # Release build optimizations
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        add_compile_options(/O2)
    endif()
endif()

# Print configuration info
message(STATUS "Horcrux v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}")

# Add subdirectories
add_subdirectory(src/core)

# Optional: Add benchmarks (disabled by default for faster builds)
option(HORCRUX_BUILD_BENCHMARKS "Build benchmark executables" ON)
if(HORCRUX_BUILD_BENCHMARKS)
    message(STATUS "Building benchmarks")
    add_subdirectory(benchmarks)
endif()

# Installation rules
install(TARGETS horcrux
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
