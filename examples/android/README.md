# Android Examples

A collection of Android application examples that demonstrate how to use Horcrux as the build system for Android projects. Each example is self-contained and showcases different Android development patterns.

## Examples

| Example | Description |
|---------|-------------|
| [basic-xml-app](basic-xml-app/) | Traditional Android app with XML layouts and Activities |
| [compose-app](compose-app/) | Modern Jetpack Compose UI with Navigation and Material 3 |
| [ndk-app](ndk-app/) | Android app with native C++ code via JNI (multi-ABI) |
| [multi-flavor-app](multi-flavor-app/) | App with product flavors (free/paid) and build variants |
| [multi-module-app](multi-module-app/) | Complex multi-module project with feature and library modules |

## Prerequisites

All examples require:

- **Android SDK** with API 34 (compileSdk)
- **Java 17+**
- **Horcrux build system** (latest)

For NDK examples additionally:
- **Android NDK** r21 or later (r26+ recommended)

### Environment Setup

```bash
export ANDROID_HOME=/path/to/android-sdk
export ANDROID_NDK_ROOT=/path/to/android-ndk   # for ndk-app only

# Verify your toolchain
horcrux doctor android
```

## Building Examples

Each example follows the same pattern:

```bash
# Navigate to an example
cd examples/android/basic-xml-app

# Or use Horcrux labels from the workspace root
horcrux build //examples/android/basic-xml-app:debug
horcrux build //examples/android/basic-xml-app:release

# Run tests
horcrux test //examples/android/basic-xml-app/...
```

## Build File Format

Every example uses Horcrux's Starlark-style `BUILD` files and a `horcrux.yaml` workspace configuration:

- **`BUILD`** — Declares build targets using rules like `android_binary`, `android_library`, `android_apk`, and `cc_library`
- **`horcrux.yaml`** — Workspace-level configuration: SDK versions, flavors, NDK settings, Compose options, and module layout

## Example Walkthrough

### Basic XML App

```python
# BUILD
android_binary(
    name = "app",
    srcs = glob(["src/main/java/**/*.java"]),
    manifest = "src/main/AndroidManifest.xml",
    resource_files = glob(["src/main/res/**"]),
    deps = ["@maven//:androidx_appcompat_appcompat"],
    min_sdk_version = 21,
    target_sdk_version = 34,
)

android_apk(
    name = "debug",
    app = ":app",
    keystore = "//tools/keystore:debug_keystore",
)
```

```yaml
# horcrux.yaml
workspace:
  name: basic-xml-app
  type: android-app
android:
  compileSdk: 34
  applicationId: com.example.basicxml
```

## Further Reading

- [Android Toolchain Detection](../../docs/android-toolchain.md)
- [Android Compiler Rules](../../docs/android-compiler-rules.md)
- [Android NDK Build Support](../../docs/android-ndk-build.md)
- [Jetpack Compose Support](../../docs/android-compose-support.md)
- [Multi-Variant Build Support](../../docs/android-variant-support.md)
- [APK & AAB Packaging](../../docs/android-apk-aab-packaging.md)
