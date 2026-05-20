cmake_minimum_required(VERSION 3.25)

project(Horcrux
    VERSION 2026.0401.0
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

# Find OpenSSL for SHA256 hashing
find_package(OpenSSL REQUIRED)

set(EXPECTED_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    tl_expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
    GIT_TAG v1.1.0
)
FetchContent_MakeAvailable(tl_expected)

# Add TinyXML2 for XML parsing (manifest merger)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 10.0.0
)
FetchContent_MakeAvailable(tinyxml2)

# Add nlohmann/json for JSON serialization/deserialization
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# ---- Submodules ----
add_subdirectory(src/core)
add_subdirectory(src/cli)

# ---- Testing ----
option(HORCRUX_BUILD_TESTS "Build unit tests" ON)
if(HORCRUX_BUILD_TESTS)
    enable_testing()
    include(CTest)
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
