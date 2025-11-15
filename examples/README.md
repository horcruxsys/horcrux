# Horcrux Examples

This directory contains example projects demonstrating Horcrux build system usage.

## Examples

### Hello Example

A simple C++ application with a library dependency:

```bash
# Build the hello app
horcrux build //examples/hello:app

# Build just the library
horcrux build //examples/hello:lib
```

**Structure:**
- `main.cpp` - Application entry point
- `lib.cpp`, `lib.h` - Simple greeting library
- `BUILD` - Build configuration

### Simple Example

A minimal single-file application:

```bash
# Build the simple app
horcrux build //examples/simple:app
```

**Structure:**
- `main.cpp` - Application entry point
- `BUILD` - Build configuration

### JNI Hello Example

A comprehensive Android JNI (Java Native Interface) example demonstrating native C++ library development for all Android ABIs:

```bash
cd examples/jni-hello

# Build for all ABIs (arm64-v8a, armeabi-v7a, x86, x86_64)
./build.sh

# Or build for specific ABI using CMake
cmake -S . -B build/arm64-v8a \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21
cmake --build build/arm64-v8a
```

**Features:**
- Multi-file C++ JNI library
- Mathematical utilities (factorial, prime check, Fibonacci)
- String manipulation demos
- CMake-based build system
- Support for all 4 Android ABIs
- Reproducible builds

**Structure:**
- `cpp/hello_jni.cpp` - Main JNI functions
- `cpp/math_utils.cpp/h` - C++ utility functions
- `cpp/math_jni.cpp` - JNI bindings for math utilities
- `CMakeLists.txt` - CMake build configuration
- `build.sh` - Build script for all ABIs
- `README.md` - Detailed documentation

See [jni-hello/README.md](jni-hello/README.md) for detailed usage instructions.

## Testing Cache Behavior

Run the same build command twice to see caching in action:

```bash
# First build (slower - compiles from source)
horcrux build //examples/hello:app

# Second build (instant - uses cache)
horcrux build //examples/hello:app
```

## Verbose Mode

Enable verbose logging to see detailed build information:

```bash
horcrux build //examples/hello:app --verbose
```
