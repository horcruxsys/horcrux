cmake_minimum_required(VERSION 3.25)

project(Horcrux
    VERSION 0.1.0
    DESCRIPTION "The Next-Generation Universal Build System"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ----- Print diagnostic -----
message(STATUS "Horcrux v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")

# ---- Dependencies ----
include(FetchContent)

set(EXPECTED_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    tl_expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
    GIT_TAG v1.1.0
)
FetchContent_MakeAvailable(tl_expected)

# ---- Submodules ----
add_subdirectory(src/core)

option(HORCRUX_BUILD_TESTS "Build unit tests" ON)
if(HORCRUX_BUILD_TESTS)
    add_subdirectory(tests)
endif()

option(HORCRUX_BUILD_BENCHMARKS "Build benchmarks" ON)
if(HORCRUX_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

install(TARGETS horcrux
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
