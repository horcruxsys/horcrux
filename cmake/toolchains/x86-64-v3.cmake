# Horcrux x86-64-v3 Toolchain  
# Requires: AVX2, FMA, BMI1, BMI2
# Compatible with: Intel Haswell (2013+), AMD Excavator (2015+)
# Good for: Modern servers and workstations

set(CMAKE_SYSTEM_NAME Linux)

# x86-64-v3 microarchitecture level
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=x86-64-v3 -mtune=generic -ffast-math -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -march=x86-64-v3 -mtune=generic")

# Enable LTO for production builds
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# Standard warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-missing-field-initializers")

message(STATUS "Using x86-64-V3 toolchain: AVX2 + FMA + BMI")