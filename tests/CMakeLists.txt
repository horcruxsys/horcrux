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

# Android NDK compiler tests
add_executable(android_ndk_compiler_test
    android_ndk_compiler_test.cpp
)

target_link_libraries(android_ndk_compiler_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_ndk_compiler_test PRIVATE
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
gtest_discover_tests(android_ndk_compiler_test)

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

# Gradle parser tests
add_executable(gradle_parser_test
    gradle_parser_test.cpp
)

target_link_libraries(gradle_parser_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(gradle_parser_test PRIVATE
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
gtest_discover_tests(gradle_parser_test)

# Workspace config generator tests
add_executable(workspace_config_generator_test
    workspace_config_generator_test.cpp
)

target_link_libraries(workspace_config_generator_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(workspace_config_generator_test PRIVATE
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
gtest_discover_tests(workspace_config_generator_test)

# Import command tests
add_executable(import_command_test
    import_command_test.cpp
)

target_link_libraries(import_command_test
    PRIVATE
    horcrux_cli
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(import_command_test PRIVATE
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
gtest_discover_tests(import_command_test)

# Android variant tests
add_executable(android_variant_test
    android_variant_test.cpp
)

target_link_libraries(android_variant_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_variant_test PRIVATE
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
gtest_discover_tests(android_variant_test)

# Android sandbox tests
add_executable(android_sandbox_test
    android_sandbox_test.cpp
)

target_link_libraries(android_sandbox_test
    PRIVATE
    horcrux_core
    gtest
    gtest_main
)

target_compile_options(android_sandbox_test PRIVATE
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
gtest_discover_tests(android_sandbox_test)

# Custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS build_graph_test local_cache_test integration_test android_toolchain_test android_ndk_compiler_test android_java_compiler_test android_kotlin_compiler_test android_compose_compiler_test android_resources_test android_manifest_merger_test android_dex_compiler_test android_apk_packager_test android_aab_packager_test gradle_parser_test workspace_config_generator_test import_command_test android_variant_test android_sandbox_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all unit tests"
)
