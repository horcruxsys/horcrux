# Android APK & AAB Packaging

This document describes Horcrux's Android APK and AAB packaging system, which provides the final step in the Android build pipeline to produce distributable application packages.

## Overview

Horcrux provides comprehensive support for creating both APK (Android Package) and AAB (Android App Bundle) files, the two standard distribution formats for Android applications.

### APK Packaging

APK files are the traditional format for distributing Android applications. They contain all the components needed to install and run an app on an Android device.

### AAB Packaging

Android App Bundles (AAB) are the modern publishing format that enables Google Play to generate optimized APKs for different device configurations, reducing download sizes and improving user experience.

## Features

### APK Packager

- **Complete Packaging**: Combines resources, DEX files, native libraries, and assets into a single APK
- **Zipalign**: Aligns file boundaries for optimal runtime performance
- **Signing**: Sign APKs with debug or release keystores (v1, v2, v3, v4 schemes)
- **Verification**: Validate APK signatures using apksigner
- **Deterministic Builds**: Compute packaging hashes for build caching

### AAB Packager

- **Multi-Module Support**: Create bundles with base and feature modules
- **Dynamic Delivery**: Support for on-demand and conditional module delivery
- **Universal APK**: Generate universal APKs for testing or sideloading
- **Bundletool Integration**: Automatic bundletool usage for bundle creation
- **Module Validation**: Ensure AAB structure integrity

## Usage

### Building an APK

```cpp
#include "android_apk_packager.h"

using namespace horcrux::core;

// Create packager with Android toolchain
AndroidApkPackager packager(toolchain);

// Configure APK packaging
ApkPackagingConfig config;
config.resources_apk = "build/resources.ap_";
config.dex_files = {"build/classes.dex", "build/classes2.dex"};
config.native_libs = {
    "build/lib/armeabi-v7a/libnative.so",
    "build/lib/arm64-v8a/libnative.so"
};
config.assets = {"assets/data.json"};
config.output_apk = "build/app-debug.apk";
config.zipalign = true;

// Optional: Add signing configuration
ApkSigningConfig signing;
signing.keystore_path = "debug.keystore";
signing.keystore_password = "android";
signing.key_alias = "androiddebugkey";
signing.key_password = "android";
config.signing_config = signing;

// Package APK
auto result = packager.package(config);
if (result) {
    std::cout << "APK created: " << result->output_apk << "\n";
    std::cout << "Signed: " << result->is_signed << "\n";
    std::cout << "Verified: " << result->is_verified << "\n";
} else {
    std::cerr << "Packaging failed: " << to_string(result.error()) << "\n";
}
```

### Building an AAB

```cpp
#include "android_aab_packager.h"

using namespace horcrux::core;

// Create packager with Android toolchain
AndroidAabPackager packager(toolchain);

// Configure base module
AabModuleConfig base_module;
base_module.module_name = "base";
base_module.manifest = "build/AndroidManifest.xml";
base_module.resources_apk = "build/resources.ap_";
base_module.dex_files = {"build/classes.dex"};
base_module.is_base_module = true;

// Configure feature module (optional)
AabModuleConfig feature_module;
feature_module.module_name = "premium";
feature_module.manifest = "build/feature/AndroidManifest.xml";
feature_module.resources_apk = "build/feature/resources.ap_";
feature_module.dex_files = {"build/feature/classes.dex"};
feature_module.is_base_module = false;

// Configure AAB packaging
AabPackagingConfig config;
config.modules = {base_module, feature_module};
config.output_aab = "build/app.aab";

// Package AAB
auto result = packager.package(config);
if (result) {
    std::cout << "AAB created: " << result->output_aab << "\n";
    std::cout << "Modules: ";
    for (const auto& module : result->module_names) {
        std::cout << module << " ";
    }
    std::cout << "\n";
} else {
    std::cerr << "Packaging failed: " << to_string(result.error()) << "\n";
}
```

### Generating Universal APK from AAB

```cpp
#include "android_aab_packager.h"

using namespace horcrux::core;

AndroidAabPackager packager(toolchain);

// Configure universal APK generation
UniversalApkConfig config;
config.aab_path = "build/app.aab";
config.output_apk = "build/app-universal.apk";

// Optional: Add signing
config.keystore_path = "release.keystore";
config.keystore_password = "password";
config.key_alias = "release";
config.key_password = "password";

// Generate universal APK
auto result = packager.generate_universal_apk(config);
if (result) {
    std::cout << "Universal APK created: " << result->output_apk << "\n";
} else {
    std::cerr << "Generation failed: " << to_string(result.error()) << "\n";
}
```

## Command Line Interface

### Build Debug APK

```bash
horcrux-cli build //app:debug_apk
```

### Build Release APK

```bash
horcrux-cli build //app:release_apk --signing-config=release.properties
```

### Build AAB

```bash
horcrux-cli build //app:bundle
```

### Generate Universal APK

```bash
horcrux-cli build //app:universal_apk --from-bundle=app.aab
```

## Build Configuration

### Defining APK Target in BUILD File

```python
android_app(
    name = "app",
    manifest = "AndroidManifest.xml",
    srcs = glob(["src/**/*.java"]),
    res = "res",
    deps = [
        "//libs:support",
    ],
    # APK-specific configuration
    apk_name = "MyApp",
    version_code = 1,
    version_name = "1.0.0",
)

# Debug APK
android_apk(
    name = "debug_apk",
    app = ":app",
    keystore = "debug.keystore",
    keystore_password = "android",
    key_alias = "androiddebugkey",
)

# Release APK
android_apk(
    name = "release_apk",
    app = ":app",
    keystore = "release.keystore",
    keystore_password_file = ".keystore_password",
    key_alias = "release",
)
```

### Defining AAB Target in BUILD File

```python
android_bundle(
    name = "bundle",
    base = ":app",
    features = [
        "//features:premium",
        "//features:extra",
    ],
)
```

## Architecture

### APK Structure

```
app.apk (ZIP file)
├── AndroidManifest.xml        # Manifest
├── classes.dex                # Primary DEX
├── classes2.dex               # Secondary DEX (multi-dex)
├── res/                       # Compiled resources
├── resources.arsc             # Resource table
├── assets/                    # Asset files
├── lib/                       # Native libraries
│   ├── armeabi-v7a/
│   │   └── libnative.so
│   ├── arm64-v8a/
│   │   └── libnative.so
│   ├── x86/
│   │   └── libnative.so
│   └── x86_64/
│       └── libnative.so
└── META-INF/                  # Signatures
    ├── MANIFEST.MF
    ├── CERT.SF
    └── CERT.RSA
```

### AAB Structure

```
app.aab (ZIP file)
├── base/                      # Base module (required)
│   ├── manifest/
│   │   └── AndroidManifest.xml
│   ├── dex/
│   │   └── classes.dex
│   ├── res/
│   ├── root/                  # Resources.ap_ contents
│   └── native.pb              # Native library metadata
├── feature1/                  # Feature module
│   ├── manifest/
│   │   └── AndroidManifest.xml
│   └── dex/
│       └── classes.dex
├── BundleConfig.pb            # Bundle configuration
└── BUNDLE-METADATA/           # Bundle metadata
    └── com.android.tools.build.obfuscation/
```

## Signing

### Keystore Creation

Create a debug keystore (for development):

```bash
keytool -genkey -v -keystore debug.keystore \
    -alias androiddebugkey \
    -keyalg RSA -keysize 2048 -validity 10000 \
    -storepass android -keypass android
```

Create a release keystore (for production):

```bash
keytool -genkey -v -keystore release.keystore \
    -alias release \
    -keyalg RSA -keysize 2048 -validity 10000
```

### Signature Schemes

Horcrux supports all Android signature schemes:

- **v1 (JAR)**: Legacy signature scheme (APK as JAR file)
- **v2**: APK Signature Scheme v2 (faster verification, API 24+)
- **v3**: APK Signature Scheme v3 (key rotation, API 28+)
- **v4**: APK Signature Scheme v4 (streaming installation, API 30+)

### Verification

Verify an APK signature:

```bash
# Using Android SDK tools
apksigner verify --verbose app.apk

# Using Horcrux
horcrux-cli verify //app:release_apk
```

## Zipalign

Zipalign optimizes APK files by aligning uncompressed data to 4-byte boundaries, enabling memory-mapping of resources at runtime for better performance.

### Benefits

- Reduced memory consumption
- Faster app startup
- Better runtime performance
- Required for app store distribution

### Alignment

Horcrux automatically zipaligns all APKs:

```cpp
config.zipalign = true;           // Enable (default)
config.zipalign_alignment = 4;    // 4-byte alignment (default)
```

## Bundletool

Bundletool is Google's official tool for working with Android App Bundles. Horcrux integrates bundletool for AAB operations.

### Automatic Detection

Horcrux searches for bundletool in:

1. `$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/bundletool.jar`
2. `$ANDROID_SDK_ROOT/tools/bin/bundletool.jar`
3. `/usr/local/bin/bundletool.jar`
4. `$HOME/.android/bundletool.jar`

### Manual Configuration

Specify bundletool location:

```cpp
AabPackagingConfig config;
config.bundletool_jar = "/path/to/bundletool.jar";
```

### Download Bundletool

Download bundletool automatically:

```cpp
AndroidAabPackager packager(toolchain);
auto result = packager.download_bundletool("/path/to/bundletool.jar");
```

## Performance

### Build Times

Typical packaging times on a modern development machine:

| Operation | Time |
|-----------|------|
| APK packaging (small app) | 100-300 ms |
| APK packaging (large app) | 500-1000 ms |
| Zipalign | 50-100 ms |
| APK signing | 100-200 ms |
| AAB packaging | 200-500 ms |
| Universal APK from AAB | 500-1000 ms |

### Caching

Horcrux caches packaging operations based on input hashes:

```cpp
// Compute deterministic hash for caching
std::string hash = AndroidApkPackager::compute_packaging_hash(config);
```

Cached packages are reused if:
- Resources haven't changed
- DEX files haven't changed
- Native libraries haven't changed
- Signing configuration is identical

## Troubleshooting

### APK Verification Failed

**Problem**: `apksigner verify` fails

**Solutions**:
- Ensure keystore exists and password is correct
- Check signature scheme compatibility (v1 for older Android)
- Verify zipalign was run before signing

### Zipalign Not Found

**Problem**: `zipalign tool not found`

**Solutions**:
- Ensure Android SDK build-tools are installed
- Set `ANDROID_HOME` or `ANDROID_SDK_ROOT` environment variable
- Install latest build-tools via SDK Manager

### Bundletool Not Found

**Problem**: `bundletool not found`

**Solutions**:
- Download bundletool from [GitHub releases](https://github.com/google/bundletool/releases)
- Place in one of the search paths or specify with `config.bundletool_jar`
- Use `packager.download_bundletool()` to download automatically

### Universal APK Generation Failed

**Problem**: Universal APK generation fails

**Solutions**:
- Ensure AAB file exists and is valid
- Check Java is installed (`java -version`)
- Verify bundletool.jar is accessible
- Check signing configuration if provided

## See Also

- [Android Resources](android-resources.md) - Resource compilation with AAPT2
- [Android DEX Compiler](android-compiler-rules.md) - DEX compilation
- [Android Toolchain](android-toolchain.md) - Toolchain detection
- [Architecture](architecture.md) - Overall system architecture
