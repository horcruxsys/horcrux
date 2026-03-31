# Horcrux UndefinedBehaviorSanitizer Toolchain
# Used for release-candidate quality gates.
#
# Usage:
#   cmake -B build-ubsan \
#     -DCMAKE_BUILD_TYPE=Debug \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ubsan.cmake
#   cmake --build build-ubsan -j$(nproc)
#   ctest --test-dir build-ubsan --output-on-failure

set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_CXX_FLAGS_DEBUG "-O1 -g -fno-omit-frame-pointer -march=x86-64 -mtune=generic")
set(CMAKE_CXX_FLAGS_RELEASE "-O1 -g -fno-omit-frame-pointer -march=x86-64 -mtune=generic -DNDEBUG")

# UBSan flags — treat all UB as hard errors
set(UBSAN_FLAGS
    "-fsanitize=undefined,null,signed-integer-overflow,bounds,alignment,object-size"
    "-fno-sanitize-recover=all"
    "-fsanitize-undefined-trap-on-error"
)
list(JOIN UBSAN_FLAGS " " UBSAN_FLAGS_STR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${UBSAN_FLAGS_STR} -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${UBSAN_FLAGS_STR}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${UBSAN_FLAGS_STR}")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

message(STATUS "Using UBSan toolchain: UndefinedBehaviorSanitizer enabled (hard errors)")
