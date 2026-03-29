# NDK App

An Android application with native C++ code via JNI, demonstrating Horcrux's NDK build support for multi-ABI native libraries.

## Overview

This example demonstrates:
- JNI (Java Native Interface) integration
- Multi-ABI builds (arm64-v8a, armeabi-v7a, x86_64)
- `cc_library` rule for native shared libraries
- Native library packaging into the APK
- Hermetic NDK toolchain management

## Project Structure

```
ndk-app/
├── BUILD                             # Horcrux build targets
├── horcrux.yaml                      # Workspace configuration
├── proguard-rules.pro
├── src/
│   └── main/
│       ├── AndroidManifest.xml
│       ├── cpp/
│       │   └── native_lib.cpp        # JNI implementation
│       ├── java/com/example/ndk/
│       │   └── MainActivity.java
│       └── res/
│           ├── layout/activity_main.xml
│           └── values/strings.xml
└── README.md
```

## Building

```bash
# Debug APK (all ABIs)
horcrux build //examples/android/ndk-app:debug

# Release APK (all ABIs)
horcrux build //examples/android/ndk-app:release

# Build native library only for a single ABI
horcrux build //examples/android/ndk-app:native_lib --abi arm64-v8a

# Run unit tests
horcrux test //examples/android/ndk-app/...
```

## Prerequisites

- Android SDK with API 34
- Android NDK r21+ (r26 recommended)
- Java 17+
- Horcrux build system

Set up environment:
```bash
export ANDROID_HOME=/path/to/android-sdk
export ANDROID_NDK_ROOT=/path/to/android-ndk
horcrux doctor android
```

## Key Concepts Demonstrated

- `cc_library` rule with `target_compatible_with = ["@platforms//os:android"]`
- `android_binary` with `native_libs` and `ndk_abis` attributes
- `horcrux.yaml` with `ndk` configuration section (abiFilters, cppStandard, apiLevel)
- JNI function naming convention: `Java_<package>_<class>_<method>`
