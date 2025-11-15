# Android SDK & NDK Toolchain Detection

This document describes Horcrux's Android toolchain detection system, which provides hermetic and reproducible builds for Android projects.

## Overview

Horcrux automatically detects and validates Android SDK, NDK, and Java toolchains across different platforms (Linux, macOS, Windows/WSL). The detection system ensures that builds are reproducible by computing a Merkle hash of the entire toolchain configuration.

## Features

- **Automatic Detection**: Discovers Android SDK, NDK, and Java installations from environment variables or common locations
- **Version Management**: Tracks multiple versions of build-tools, platforms, and NDKs
- **Cross-Platform**: Works on Linux, macOS, and Windows (WSL)
- **Reproducibility**: Generates a toolchain manifest with Merkle hash for hermetic builds
- **Validation**: Comprehensive checks to ensure toolchain integrity

## Usage

### Check Android Toolchain

To validate your Android toolchain setup, run:

```bash
horcrux doctor android
```

This command will:
1. Detect your Android SDK, NDK, and Java installations
2. Validate the structure and availability of required components
3. Display detailed information about detected versions
4. Save a toolchain manifest to `.horcrux/lock/android-toolchain.json`
5. Compute a Merkle hash for reproducibility

### Example Output

```
✓ Android Toolchain Detected

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Android SDK
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    SDK Root: /home/user/Android/Sdk
    Platform Tools: /home/user/Android/Sdk/platform-tools
    Command Line Tools: /home/user/Android/Sdk/cmdline-tools

Build Tools (3 version(s) found):
    Version 34.0.0: /home/user/Android/Sdk/build-tools/34.0.0
    Version 33.0.2: /home/user/Android/Sdk/build-tools/33.0.2
    Version 30.0.3: /home/user/Android/Sdk/build-tools/30.0.3

Platforms (4 API level(s) found):
    API Level 34: /home/user/Android/Sdk/platforms/android-34
    API Level 33: /home/user/Android/Sdk/platforms/android-33
    API Level 30: /home/user/Android/Sdk/platforms/android-30
    API Level 28: /home/user/Android/Sdk/platforms/android-28

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Android NDK
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    NDK Root: /home/user/Android/Sdk/ndk

NDK Versions (2 version(s) found):
    Version 26.0.0: /home/user/Android/Sdk/ndk/26.0.0
    Version 25.2.9519653: /home/user/Android/Sdk/ndk/25.2.9519653

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Java SDK
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    Java Version: 17.0.0
    JAVA_HOME: /usr/lib/jvm/java-17-openjdk
    javac: /usr/lib/jvm/java-17-openjdk/bin/javac
    java: /usr/lib/jvm/java-17-openjdk/bin/java

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Reproducibility
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    Toolchain Hash: a1b2c3d4e5f6...

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Validation
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  [✓] Toolchain configuration valid (PASS)
  [✓] SDK structure valid (PASS)
  [✓] NDK structure valid (PASS)
  [✓] Java installation valid (PASS)

✓ Toolchain manifest saved to: .horcrux/lock/android-toolchain.json
  This file ensures reproducible builds across machines.

✓ Android toolchain fully configured
  All components detected successfully!
```

## Environment Variables

Horcrux detects Android toolchains using the following environment variables (in order of precedence):

### Android SDK
1. `ANDROID_HOME` (preferred)
2. `ANDROID_SDK_ROOT`
3. Common installation paths:
   - `$HOME/Android/Sdk` (Linux)
   - `$HOME/Library/Android/sdk` (macOS)
   - `/usr/local/android-sdk`
   - `/opt/android-sdk`

### Android NDK
1. `ANDROID_NDK_ROOT` (preferred)
2. `ANDROID_NDK_HOME`
3. `$ANDROID_SDK_ROOT/ndk-bundle`
4. `$ANDROID_SDK_ROOT/ndk/*` (versioned NDK directories)

### Java SDK
1. `JAVA_HOME` (required for Java/Kotlin compilation)

## Components Detected

### Build Tools
Located in `$ANDROID_SDK_ROOT/build-tools/`, includes:
- `aapt` (Android Asset Packaging Tool)
- `aidl` (Android Interface Definition Language)
- `dx` (DEX compiler)
- `zipalign` (Archive alignment tool)

### Platforms
Located in `$ANDROID_SDK_ROOT/platforms/`, includes:
- `android.jar` (Android framework classes)
- Platform-specific resources

### NDK
Located in `$ANDROID_NDK_ROOT/`, includes:
- Toolchains for native compilation
- Support for multiple ABIs: `armeabi-v7a`, `arm64-v8a`, `x86`, `x86_64`

### Platform Tools
Located in `$ANDROID_SDK_ROOT/platform-tools/`, includes:
- `adb` (Android Debug Bridge)
- `fastboot`

### Command Line Tools
Located in `$ANDROID_SDK_ROOT/cmdline-tools/`, includes:
- `sdkmanager`
- `avdmanager`

## Toolchain Manifest

The toolchain manifest is saved to `.horcrux/lock/android-toolchain.json` and contains:

```json
{
  "sdk_root": "/path/to/android/sdk",
  "ndk_root": "/path/to/android/ndk",
  "java_sdk": {
    "version": "17.0.0",
    "java_home": "/path/to/java"
  },
  "build_tools": [
    {
      "version": "34.0.0",
      "path": "/path/to/android/sdk/build-tools/34.0.0"
    }
  ],
  "platforms": [
    {
      "api_level": "34",
      "version": "14.0",
      "path": "/path/to/android/sdk/platforms/android-34"
    }
  ],
  "ndks": [
    {
      "version": "26.0.0",
      "path": "/path/to/android/ndk/26.0.0"
    }
  ],
  "merkle_hash": "a1b2c3d4e5f6..."
}
```

This manifest ensures that:
1. All team members use the same toolchain versions
2. CI/CD builds are reproducible
3. Builds can be verified across different machines

## Troubleshooting

### SDK Not Found

If you see "Android SDK not found", ensure that:
1. Android SDK is installed
2. `ANDROID_HOME` or `ANDROID_SDK_ROOT` environment variable is set
3. The SDK directory contains `build-tools` and `platforms` subdirectories

```bash
export ANDROID_HOME=/path/to/android/sdk
horcrux doctor android
```

### NDK Not Found (Optional)

NDK is optional unless you're building native code. To use NDK:
1. Install NDK via Android SDK Manager
2. Set `ANDROID_NDK_ROOT` or ensure NDK is in `$ANDROID_SDK_ROOT/ndk/`

### Java Not Found (Optional)

Java is optional unless you're compiling Java/Kotlin code. To use Java:
1. Install Java Development Kit (JDK)
2. Set `JAVA_HOME` environment variable

```bash
export JAVA_HOME=/path/to/jdk
horcrux doctor android
```

## Architecture

The Android toolchain detection system consists of:

### Core Components

1. **AndroidToolchainDetector** (`src/core/android_toolchain.h`)
   - Automatic detection from environment variables
   - Scanning of SDK components
   - Version discovery

2. **AndroidToolchainValidator** (`src/core/android_toolchain.h`)
   - SDK structure validation
   - NDK structure validation
   - Java installation validation

3. **AndroidToolchain** (`src/core/android_toolchain.h`)
   - Data structure for toolchain configuration
   - JSON serialization/deserialization
   - Merkle hash computation

### CLI Interface

4. **Doctor Command** (`src/cli/doctor_command.h`)
   - User-facing interface
   - Pretty-printed output
   - Manifest generation

## Future Enhancements

- [ ] Automatic SDK download and installation
- [ ] SDK component version pinning in BUILD files
- [ ] Remote toolchain caching
- [ ] Toolchain verification from manifest
- [ ] Auto-update notifications for outdated components

## See Also

- [Coding Standards](coding-standards.md)
- [CLI Implementation](cli-implementation.md)
- [Architecture](architecture.md)
