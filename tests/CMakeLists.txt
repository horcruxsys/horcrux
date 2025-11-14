# Tests CMakeLists.txt
# Unit tests for Horcrux

# Fetch Google Test
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Configure gtest options
set(BUILD_GMOCK OFF CACHE BOOL "Disable GMock" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable GTest install" FORCE)

# Make googletest available
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Build graph tests
add_executable(build_graph_test
    build_graph_test.cpp
)

target_link_libraries(build_graph_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(build_graph_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:
        -std=c++23
        -Wall -Wextra
    >
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:
        -std=c++23
        -Wall -Wextra
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
    >
)

# Register test with CTest
include(GoogleTest)
gtest_discover_tests(build_graph_test)

# Custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS build_graph_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all unit tests"
)
