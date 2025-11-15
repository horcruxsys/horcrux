# Horcrux Build Toolchains

This directory contains production-grade CMake toolchain files for different build scenarios and CPU targets.

## 🚀 Quick Start

### Local Development (Maximum Performance)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/local.cmake
cmake --build build -j$(nproc)
```

### CI/CD Builds (Portable & Safe)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ci.cmake
cmake --build build -j$(nproc)
```

### Production Deployment

```bash
# For most servers (AVX2 support)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86-64-v3.cmake

# For maximum compatibility
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86-64.cmake
```

## 📋 Available Toolchains

| Toolchain | Target | CPU Features | Use Case |
|-----------|--------|--------------|----------|
| `local.cmake` | Native CPU | All available | Local development |
| `ci.cmake` | x86-64 baseline | Basic x86-64 | GitHub Actions, CI/CD |
| `x86-64.cmake` | x86-64 baseline | Basic x86-64 | Maximum compatibility |
| `x86-64-v2.cmake` | x86-64-v2 | SSE4.2, POPCNT | Most servers (2008+) |
| `x86-64-v3.cmake` | x86-64-v3 | AVX2, FMA, BMI | Modern servers (2013+) |
| `x86-64-v4.cmake` | x86-64-v4 | AVX512 | HPC workloads (2017+) |

## 🔧 Technical Details

### Local Toolchain Features

- `-march=native -mtune=native` - Optimized for your specific CPU
- Full LTO (Link-Time Optimization) enabled
- Aggressive warnings (`-Werror=return-type`)
- Fast math optimizations

### CI Toolchain Features

- `-march=x86-64 -mtune=generic` - Portable baseline
- LTO disabled (prevents illegal instructions on different CI hardware)
- Conservative warning levels
- Compatible with third-party dependencies

### Production Toolchains

- Specific CPU microarchitecture levels
- LTO enabled for maximum performance
- Balanced optimization vs compatibility

## 💡 Performance Tips

1. **Local Development**: Use `local.cmake` for maximum performance during development
2. **CI/CD**: Always use `ci.cmake` to prevent build failures on different hardware
3. **Production**: Choose the highest CPU tier your target hardware supports
4. **Docker**: Use `x86-64.cmake` for maximum container portability

## 🧪 Benchmarking Different Toolchains

```bash
# Benchmark with local optimizations
cmake -B build-local -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/local.cmake
cmake --build build-local
./build-local/bin/dependency_resolution_bench

# Benchmark with portable build
cmake -B build-portable -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86-64.cmake
cmake --build build-portable
./build-portable/bin/dependency_resolution_bench
```

## 🛠 Adding Custom Toolchains

To create a custom toolchain:

1. Copy an existing toolchain file
2. Modify the `CMAKE_CXX_FLAGS_RELEASE` for your target
3. Update the architecture flags (`-march`, `-mtune`)
4. Test thoroughly on your target hardware

Example for ARM64:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=armv8-a -mtune=generic -ffast-math -DNDEBUG")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
```

## 📚 References

- [x86-64 microarchitecture levels](https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels)
- [GCC optimization options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [CMake toolchain files](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)
