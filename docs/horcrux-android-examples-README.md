# 🤖 Horcrux Android Examples

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build Status](https://github.com/horcruxsys/horcrux-android-examples/actions/workflows/build-all-examples.yml/badge.svg)](https://github.com/horcruxsys/horcrux-android-examples/actions/workflows/build-all-examples.yml)
[![Horcrux Version](https://img.shields.io/badge/Horcrux-0.1.x-blue.svg)](https://github.com/horcruxsys/horcrux)

Production-ready Android example projects demonstrating the [Horcrux build system](https://github.com/horcruxsys/horcrux)'s comprehensive Android development capabilities.

## 📋 Overview

This repository contains real-world Android applications that showcase:

- ✅ **Various Android architectures** (XML layouts, Jetpack Compose, NDK)
- ✅ **Build system features** (flavors, variants, multi-module projects)
- ✅ **Production patterns** (testing, CI/CD, optimization)
- ✅ **Migration guides** (Gradle to Horcrux conversion)

These examples serve as:
- 📚 **Learning resources** for developers adopting Horcrux
- 🧪 **Integration tests** for the Horcrux build system
- 📖 **Reference implementations** of best practices
- 🔄 **Migration templates** from Gradle to Horcrux

## 🎯 Quick Start

### Prerequisites

1. **Install Horcrux** (version 0.1.x or later)
   ```bash
   # See main repository for installation instructions
   # https://github.com/horcruxsys/horcrux
   ```

2. **Set up Android SDK and NDK**
   ```bash
   export ANDROID_HOME=/path/to/android-sdk
   export ANDROID_NDK_ROOT=/path/to/android-ndk
   ```

3. **Verify your toolchain**
   ```bash
   horcrux doctor android
   ```

### Build All Examples

```bash
# Clone the repository
git clone https://github.com/horcruxsys/horcrux-android-examples.git
cd horcrux-android-examples

# Build all examples at once
./scripts/build-all.sh

# Or run tests for all examples
./scripts/test-all.sh
```

### Build Individual Examples

```bash
# Basic XML App
cd basic-xml-app
horcrux build //app:debug

# Compose App
cd compose-app
horcrux build //app:composeDebug

# NDK App (all ABIs)
cd ndk-app
horcrux build //app:ndk-release --all-abis

# Flavor App (specific variant)
cd flavor-app
horcrux build //app:freeDebug

# Multi-Module App
cd multi-module-app
horcrux build //app:release
```

## 📱 Example Projects

### 1. Basic XML App

Traditional Android app with XML layouts demonstrating classic Android development patterns.

**Features**:
- Activities and Fragments
- XML layout resources
- Material Design components
- Resource qualifiers
- Build variants (debug/release)
- ProGuard/R8 optimization

**Location**: [`basic-xml-app/`](basic-xml-app/)

```bash
cd basic-xml-app
horcrux build //app:debug
horcrux install //app:debug  # Install to connected device
```

### 2. Jetpack Compose App

Modern Android app using 100% Jetpack Compose UI with Material 3.

**Features**:
- Compose UI toolkit
- Material 3 theming
- State management
- Compose navigation
- Animation support
- Compose compiler metrics

**Location**: [`compose-app/`](compose-app/)

```bash
cd compose-app
horcrux build //app:composeDebug --enable-compose-metrics
horcrux build //app:composeRelease
```

### 3. NDK/JNI App

Android app with native C++ code demonstrating NDK integration.

**Features**:
- JNI (Java Native Interface)
- Multi-ABI builds (arm64-v8a, armeabi-v7a, x86, x86_64)
- Native library packaging
- C++ STL integration
- Cross-compilation
- Hermetic NDK builds

**Location**: [`ndk-app/`](ndk-app/)

```bash
cd ndk-app
# Build for all ABIs
horcrux build //app:ndk-debug --all-abis

# Build for specific ABI
horcrux build //app:ndk-release --abi arm64-v8a
```

### 4. Multi-Flavor App

App with multiple product flavors and build type variants.

**Features**:
- Product flavors (free, paid, enterprise)
- Flavor dimensions (tier, api-level)
- Build types (debug, release, staging)
- Variant matrix (12 total variants)
- Per-variant resources
- Conditional dependencies

**Location**: [`flavor-app/`](flavor-app/)

```bash
cd flavor-app
# List all available variants
horcrux query //app:variants

# Build specific variant
horcrux build //app:freeDebug
horcrux build //app:paidRelease

# Build all variants
horcrux build //app:all-variants
```

### 5. Multi-Module App

Complex multi-module project with feature and library modules.

**Modules**:
- `app` - Main application module
- `feature-home` - Home feature module
- `feature-profile` - Profile feature module
- `lib-network` - Network library
- `lib-database` - Database library
- `shared-ui` - Shared UI components

**Features**:
- Module dependencies
- Feature module isolation
- Inter-module API boundaries
- Incremental compilation
- Module-level caching
- Dependency injection

**Location**: [`multi-module-app/`](multi-module-app/)

```bash
cd multi-module-app
# Build entire app
horcrux build //app:release

# Build specific library module
horcrux build //libraries/lib-network:lib

# Run all tests
horcrux test //...
```

## 🏗️ Repository Structure

```
horcrux-android-examples/
├── basic-xml-app/              # Traditional XML-based Android app
│   ├── src/
│   ├── res/
│   ├── horcrux.yaml
│   └── README.md
├── compose-app/                # Jetpack Compose app
│   ├── src/
│   ├── horcrux.yaml
│   └── README.md
├── ndk-app/                    # NDK/JNI native app
│   ├── src/
│   ├── cpp/
│   ├── horcrux.yaml
│   └── README.md
├── flavor-app/                 # Multi-flavor app
│   ├── src/
│   ├── horcrux.yaml
│   └── README.md
├── multi-module-app/           # Multi-module project
│   ├── app/
│   ├── libraries/
│   ├── features/
│   ├── shared/
│   ├── horcrux.yaml
│   └── README.md
├── docs/
│   ├── migration-guide.md      # Gradle to Horcrux migration
│   ├── best-practices.md       # Android build best practices
│   └── ci-integration.md       # CI/CD setup guide
├── scripts/
│   ├── build-all.sh            # Build all examples
│   ├── test-all.sh             # Test all examples
│   └── verify-ci.sh            # CI verification script
└── .github/
    └── workflows/
        ├── build-all-examples.yml   # CI for all examples
        └── regression-tests.yml     # Full regression suite
```

## 🔄 Gradle to Horcrux Migration

Each example includes reference Gradle configurations to help with migration:

```bash
cd basic-xml-app

# Compare Gradle and Horcrux builds
ls gradle-reference/          # Original Gradle configuration
cat horcrux.yaml             # Horcrux configuration

# Both produce identical APKs
./gradlew assembleDebug      # Gradle build
horcrux build //app:debug    # Horcrux build

# Verify outputs are equivalent
diff build/outputs/apk/debug/app-debug.apk \
     gradle-reference/build/outputs/apk/debug/app-debug.apk
```

See [`docs/migration-guide.md`](docs/migration-guide.md) for detailed migration instructions.

## 🧪 Testing

### Run All Tests

```bash
# Run unit and instrumentation tests for all examples
./scripts/test-all.sh
```

### Test Individual Examples

```bash
cd basic-xml-app

# Run unit tests
horcrux test //app:unit-tests

# Run instrumentation tests (requires connected device/emulator)
horcrux test //app:instrumentation-tests

# Run all tests
horcrux test //...
```

### Verify CI Locally

```bash
# Simulate CI environment locally
./scripts/verify-ci.sh
```

## 🚀 CI/CD Integration

This repository includes comprehensive CI workflows:

### Build All Examples

**Workflow**: `.github/workflows/build-all-examples.yml`

Triggered on:
- Every commit to `main` branch
- Pull requests
- Manual workflow dispatch

Validates:
- All examples build successfully
- Cross-platform compatibility (Linux, macOS)
- Multiple SDK/NDK version support

### Regression Tests

**Workflow**: `.github/workflows/regression-tests.yml`

Runs comprehensive tests:
- ✅ Clean builds
- ✅ Incremental builds
- ✅ Cache verification
- ✅ Multi-ABI builds
- ✅ Variant matrix expansion
- ✅ APK/AAB generation
- ✅ Build reproducibility
- ✅ Performance benchmarks

Triggered by:
- Main Horcrux repository changes
- Scheduled daily runs
- Manual workflow dispatch

## 📊 Build Performance

Typical build times on GitHub Actions (ubuntu-latest):

| Example           | Clean Build | Incremental Build | Cache Hit |
|-------------------|-------------|-------------------|-----------|
| Basic XML App     | ~30s        | ~5s               | ~2s       |
| Compose App       | ~45s        | ~8s               | ~3s       |
| NDK App (all ABIs)| ~120s       | ~15s              | ~4s       |
| Flavor App        | ~40s        | ~6s               | ~2s       |
| Multi-Module App  | ~90s        | ~12s              | ~3s       |

## 📚 Documentation

- **Main Horcrux Docs**: [github.com/horcruxsys/horcrux](https://github.com/horcruxsys/horcrux)
- **Migration Guide**: [docs/migration-guide.md](docs/migration-guide.md)
- **Best Practices**: [docs/best-practices.md](docs/best-practices.md)
- **CI Integration**: [docs/ci-integration.md](docs/ci-integration.md)
- **Android Examples Overview**: [docs/android-examples.md](https://github.com/horcruxsys/horcrux/blob/main/docs/android-examples.md)

## 🤝 Contributing

Contributions are welcome! Please see our [Contributing Guide](CONTRIBUTING.md).

### Adding a New Example

1. Create a new directory for your example
2. Implement the Android application
3. Add comprehensive `README.md`
4. Create `horcrux.yaml` configuration
5. Include tests (unit and instrumentation)
6. Update this README with the new example
7. Add CI workflow integration
8. Submit a pull request

### Improving Existing Examples

- Fix bugs
- Add tests
- Improve documentation
- Update dependencies
- Optimize build configurations

## 🐛 Troubleshooting

### Build Failures

**Issue**: Example fails to build

**Solution**:
1. Check toolchain: `horcrux doctor android`
2. Verify SDK/NDK versions match requirements
3. Clean build: `horcrux clean && horcrux build //...`
4. Review error logs for specific issues

### NDK Issues

**Issue**: NDK builds fail

**Solution**:
1. Set `ANDROID_NDK_ROOT` environment variable
2. Verify NDK version (r21+ required, r26+ recommended)
3. Check ABI support: `horcrux doctor android`

### Dependency Issues

**Issue**: Dependency resolution fails

**Solution**:
1. Check internet connectivity
2. Clear dependency cache: `horcrux clean --deps`
3. Verify `horcrux.yaml` syntax

### Installation Issues

**Issue**: Cannot install APK to device

**Solution**:
1. Check device connection: `adb devices`
2. Enable USB debugging on device
3. Try: `adb install -r build/outputs/apk/debug/app-debug.apk`

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

**Copyright © 2025 Horcrux Project Contributors**

## 🔗 Related Projects

- **Horcrux Build System**: [github.com/horcruxsys/horcrux](https://github.com/horcruxsys/horcrux)
- **Android Developer Docs**: [developer.android.com](https://developer.android.com)
- **Jetpack Compose**: [developer.android.com/jetpack/compose](https://developer.android.com/jetpack/compose)
- **Android NDK**: [developer.android.com/ndk](https://developer.android.com/ndk)

## 💬 Support

Need help?

- 📖 **Documentation**: [Main Horcrux repository](https://github.com/horcruxsys/horcrux)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/horcruxsys/horcrux/discussions)
- 🐛 **Issue Tracker**: [GitHub Issues](https://github.com/horcruxsys/horcrux/issues)
- 📧 **Email**: [maintainers@horcruxsys.org](mailto:maintainers@horcruxsys.org)

## 🌟 Acknowledgments

These examples are maintained by the Horcrux community and serve as the official reference implementations for Android development with Horcrux.

---

<p align="center">
  <strong>Build Android apps better, faster, and more reliably with Horcrux.</strong><br>
  Made with ❤️ by the Horcrux community
</p>
