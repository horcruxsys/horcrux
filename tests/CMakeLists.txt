# Tests CMakeLists.txt
# Unit tests for Horcrux

# Fetch Google Test
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
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

# Local cache tests
add_executable(local_cache_test
    local_cache_test.cpp
)

target_link_libraries(local_cache_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(local_cache_test PRIVATE
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
gtest_discover_tests(local_cache_test)

# Integration tests
add_executable(integration_test
    integration_test.cpp
)

target_link_libraries(integration_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(integration_test PRIVATE
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
gtest_discover_tests(integration_test)

# Android toolchain tests
add_executable(android_toolchain_test
    android_toolchain_test.cpp
)

target_link_libraries(android_toolchain_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_toolchain_test PRIVATE
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
gtest_discover_tests(android_toolchain_test)

# Android Java compiler tests
add_executable(android_java_compiler_test
    android_java_compiler_test.cpp
)

target_link_libraries(android_java_compiler_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_java_compiler_test PRIVATE
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
gtest_discover_tests(android_java_compiler_test)

# Android Kotlin compiler tests
add_executable(android_kotlin_compiler_test
    android_kotlin_compiler_test.cpp
)

target_link_libraries(android_kotlin_compiler_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_kotlin_compiler_test PRIVATE
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
gtest_discover_tests(android_kotlin_compiler_test)

# Android Compose compiler tests
add_executable(android_compose_compiler_test
    android_compose_compiler_test.cpp
)

target_link_libraries(android_compose_compiler_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_compose_compiler_test PRIVATE
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
gtest_discover_tests(android_compose_compiler_test)

# Android resources tests
add_executable(android_resources_test
    android_resources_test.cpp
)

target_link_libraries(android_resources_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_resources_test PRIVATE
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
gtest_discover_tests(android_resources_test)

# Android manifest merger tests
add_executable(android_manifest_merger_test
    android_manifest_merger_test.cpp
)

target_link_libraries(android_manifest_merger_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_manifest_merger_test PRIVATE
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
gtest_discover_tests(android_manifest_merger_test)

# Android DEX compiler tests
add_executable(android_dex_compiler_test
    android_dex_compiler_test.cpp
)

target_link_libraries(android_dex_compiler_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_dex_compiler_test PRIVATE
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
gtest_discover_tests(android_dex_compiler_test)

# Android APK packager tests
add_executable(android_apk_packager_test
    android_apk_packager_test.cpp
)

target_link_libraries(android_apk_packager_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_apk_packager_test PRIVATE
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
gtest_discover_tests(android_apk_packager_test)

# Android AAB packager tests
add_executable(android_aab_packager_test
    android_aab_packager_test.cpp
)

target_link_libraries(android_aab_packager_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_aab_packager_test PRIVATE
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
gtest_discover_tests(android_aab_packager_test)

# Custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS build_graph_test local_cache_test integration_test android_toolchain_test android_java_compiler_test android_kotlin_compiler_test android_compose_compiler_test android_resources_test android_manifest_merger_test android_dex_compiler_test android_apk_packager_test android_aab_packager_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all unit tests"
)
