# Horcrux ThreadSanitizer Toolchain
# Used for release-candidate quality gates.
#
# NOTE: TSan is incompatible with ASan/LSan.  Use a separate build directory.
#
# Usage:
#   cmake -B build-tsan \
#     -DCMAKE_BUILD_TYPE=Debug \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/tsan.cmake
#   cmake --build build-tsan -j$(nproc)
#   ctest --test-dir build-tsan --output-on-failure

set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_CXX_FLAGS_DEBUG "-O1 -g -fno-omit-frame-pointer -march=x86-64 -mtune=generic")
set(CMAKE_CXX_FLAGS_RELEASE "-O1 -g -fno-omit-frame-pointer -march=x86-64 -mtune=generic -DNDEBUG")

set(TSAN_FLAGS "-fsanitize=thread -fno-sanitize-recover=all")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TSAN_FLAGS} -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${TSAN_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TSAN_FLAGS}")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

message(STATUS "Using TSan toolchain: ThreadSanitizer enabled")
