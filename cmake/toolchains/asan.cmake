# Horcrux AddressSanitizer + LeakSanitizer Toolchain
# Used for release-candidate quality gates.
#
# Usage:
#   cmake -B build-asan \
#     -DCMAKE_BUILD_TYPE=Debug \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/asan.cmake
#   cmake --build build-asan -j$(nproc)
#   ctest --test-dir build-asan --output-on-failure

set(CMAKE_SYSTEM_NAME Linux)

# Disable optimisations so ASan stack traces are readable
set(CMAKE_CXX_FLAGS_DEBUG "-O1 -g -fno-omit-frame-pointer -march=x86-64 -mtune=generic")
set(CMAKE_CXX_FLAGS_RELEASE "-O1 -g -fno-omit-frame-pointer -march=x86-64 -mtune=generic -DNDEBUG")

# ASan + LSan flags
set(ASAN_FLAGS "-fsanitize=address,leak -fno-sanitize-recover=all")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS} -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ASAN_FLAGS}")

# Disable LTO — incompatible with sanitizer instrumentation
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

message(STATUS "Using ASan toolchain: AddressSanitizer + LeakSanitizer enabled")
