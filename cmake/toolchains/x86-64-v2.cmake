# Horcrux x86-64-v2 Toolchain
# Requires: SSE4.2, POPCNT
# Compatible with: Intel Nehalem (2008+), AMD Bulldozer (2011+)
# Good for: Most servers and desktops from last 15 years

set(CMAKE_SYSTEM_NAME Linux)

# x86-64-v2 microarchitecture level (2009+)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=x86-64-v2 -mtune=generic -ffast-math -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -march=x86-64-v2 -mtune=generic")

# Enable LTO for production builds
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# Standard warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-missing-field-initializers")

message(STATUS "Using x86-64-V2 toolchain: SSE4.2 + POPCNT")