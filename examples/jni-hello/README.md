# JNI Hello Example

A comprehensive example demonstrating how to build Android JNI (Java Native Interface) libraries using Horcrux's NDK compiler support.

## Overview

This example shows how to:
- Write C++ code for JNI
- Build native libraries for all Android ABIs
- Use Horcrux's NDK compiler abstraction
- Create reproducible cross-platform builds

## Project Structure

```
jni-hello/
├── cpp/
│   ├── hello_jni.cpp      # Main JNI functions
│   ├── math_utils.h       # C++ utility header
│   ├── math_utils.cpp     # C++ utility implementation
│   └── math_jni.cpp       # JNI bindings for math utilities
├── CMakeLists.txt         # CMake build configuration
├── build.sh               # Build script for all ABIs
└── README.md              # This file
```

## Features

The example includes:

1. **hello_jni.cpp** - Basic JNI functions:
   - `getGreeting()` - Returns a greeting string from C++
   - `addNumbers()` - Simple integer addition
   - `reverseString()` - String manipulation demo

2. **math_utils.cpp** - Mathematical utilities:
   - `factorial()` - Calculate factorial
   - `is_prime()` - Prime number checker
   - `fibonacci()` - Fibonacci sequence

3. **math_jni.cpp** - JNI bindings for math utilities

## Prerequisites

- Android NDK r21 or later
- CMake 3.25+
- Horcrux build system (optional, for Horcrux-specific features)

Set the NDK path:
```bash
export ANDROID_NDK_ROOT=/path/to/android-ndk
# or
export ANDROID_NDK_HOME=/path/to/android-ndk
```

## Building

### Using the Build Script

Build for all ABIs at once:

```bash
./build.sh
```

This will build for:
- arm64-v8a (64-bit ARM)
- armeabi-v7a (32-bit ARM)
- x86 (32-bit Intel)
- x86_64 (64-bit Intel)

### Using Horcrux CLI (Future)

```bash
# Generate toolchain files for all ABIs
horcrux-cli ndk generate-toolchain --all-abis --api-level 21 --output build/toolchains

# Build for specific ABI
horcrux-cli build //examples/jni-hello:hello-jni --abi arm64-v8a

# Build for all ABIs
horcrux-cli build //examples/jni-hello:hello-jni --all-abis
```

### Manual Build for Single ABI

Build for a specific ABI using CMake directly:

```bash
# Configure
cmake -S . -B build/arm64-v8a \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build/arm64-v8a -j$(nproc)
```

## Output

After building, you'll find the compiled libraries in:

```
build/libs/
├── lib/
│   ├── arm64-v8a/
│   │   └── libhello-jni.so
│   ├── armeabi-v7a/
│   │   └── libhello-jni.so
│   ├── x86/
│   │   └── libhello-jni.so
│   └── x86_64/
│       └── libhello-jni.so
```

## Using in Android Apps

### 1. Copy Libraries to Your Android Project

```bash
# Copy to Android project's jniLibs directory
cp -r build/libs/lib/* /path/to/android-project/app/src/main/jniLibs/
```

### 2. Load Native Library in Java/Kotlin

**Java:**
```java
package com.horcrux.example;

public class HelloJni {
    static {
        System.loadLibrary("hello-jni");
    }

    public native String getGreeting();
    public native int addNumbers(int a, int b);
    public native String reverseString(String input);
}
```

**Kotlin:**
```kotlin
package com.horcrux.example

class HelloJni {
    external fun getGreeting(): String
    external fun addNumbers(a: Int, b: Int): Int
    external fun reverseString(input: String): String

    companion object {
        init {
            System.loadLibrary("hello-jni")
        }
    }
}
```

### 3. Math Utilities

**Java:**
```java
package com.horcrux.example;

public class MathUtils {
    static {
        System.loadLibrary("hello-jni");
    }

    public native int factorial(int n);
    public native boolean isPrime(int n);
    public native int fibonacci(int n);
}
```

## Build Configuration

### API Level

By default, the build targets API level 21 (Android 5.0). To change this:

```bash
export ANDROID_API_LEVEL=28
./build.sh
```

### Build Type

To build in debug mode:

```bash
cmake -S . -B build/debug \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DCMAKE_BUILD_TYPE=Debug
```

## Testing

You can verify the libraries are built correctly:

```bash
# Check library architecture
file build/libs/lib/arm64-v8a/libhello-jni.so

# Check symbols
nm -D build/libs/lib/arm64-v8a/libhello-jni.so | grep Java_com_horcrux_example

# Check dependencies
objdump -p build/libs/lib/arm64-v8a/libhello-jni.so | grep NEEDED
```

## Reproducible Builds

All builds are reproducible with the same NDK version and build configuration. The Horcrux build system ensures:

- Hermetic builds (isolated from system dependencies)
- Content-addressable caching
- Deterministic output across machines

## Performance Considerations

- Uses `-O2` optimization by default for release builds
- Position Independent Code (PIC) enabled for all builds
- Stripped symbols in release builds to reduce size

## Troubleshooting

### NDK Not Found

```
Error: ANDROID_NDK_ROOT or ANDROID_NDK_HOME not set
```

**Solution:** Set the environment variable to your NDK path:
```bash
export ANDROID_NDK_ROOT=/path/to/ndk
```

### Missing JNI Headers

If compilation fails with missing JNI headers, ensure your NDK is properly installed and the version is r21 or later.

### ABI Compatibility

Ensure your target devices support the ABIs you're building for:
- arm64-v8a: Most modern Android devices (2014+)
- armeabi-v7a: Older ARM devices
- x86/x86_64: Android emulators and rare x86 devices

## Further Reading

- [Android NDK Documentation](https://developer.android.com/ndk)
- [JNI Tips and Tricks](https://developer.android.com/training/articles/perf-jni)
- [Horcrux Build System](../../README.md)
- [Android Toolchain Detection](../../docs/android-toolchain.md)

## License

Copyright (C) 2025 Horcrux Project Contributors  
Licensed under the MIT License
