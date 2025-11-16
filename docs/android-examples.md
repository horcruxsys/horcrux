# Android Examples Repository

This document describes the `horcrux-android-examples` repository, a comprehensive collection of production-ready Android example projects that demonstrate Horcrux's Android build capabilities.

## Overview

The `horcrux-android-examples` repository is a separate companion repository to the main Horcrux project, containing real-world Android application examples that showcase various Android development scenarios and serve as:

- **Learning Resources**: Practical examples for developers migrating to Horcrux
- **Integration Tests**: Full Android regression test suite run in CI
- **Reference Implementations**: Production-ready patterns for Android builds
- **Migration Guides**: Real examples of Gradle-to-Horcrux conversions

## Repository Structure

```
horcrux-android-examples/
├── README.md                          # Repository overview and quick start
├── .github/
│   └── workflows/
│       ├── build-all-examples.yml     # CI workflow for all examples
│       └── regression-tests.yml       # Full Android regression test suite
├── basic-xml-app/                     # Basic XML-based Android app
│   ├── README.md
│   ├── horcrux.yaml
│   ├── src/
│   ├── res/
│   └── AndroidManifest.xml
├── compose-app/                       # Jetpack Compose app
│   ├── README.md
│   ├── horcrux.yaml
│   ├── src/
│   └── AndroidManifest.xml
├── ndk-app/                          # NDK/JNI native app
│   ├── README.md
│   ├── horcrux.yaml
│   ├── src/
│   ├── cpp/
│   └── AndroidManifest.xml
├── flavor-app/                       # Multi-flavor app
│   ├── README.md
│   ├── horcrux.yaml
│   ├── src/
│   └── AndroidManifest.xml
├── multi-module-app/                 # Multi-module project
│   ├── README.md
│   ├── horcrux.yaml
│   ├── app/
│   ├── library1/
│   ├── library2/
│   └── shared/
├── docs/
│   ├── migration-guide.md           # Gradle to Horcrux migration
│   ├── best-practices.md            # Android build best practices
│   └── ci-integration.md            # CI/CD integration guide
└── scripts/
    ├── build-all.sh                 # Build all examples
    ├── test-all.sh                  # Test all examples
    └── verify-ci.sh                 # CI verification script
```

## Example Projects

### 1. Basic XML App

**Location**: `basic-xml-app/`

A traditional Android application using XML layouts, demonstrating:

- Basic Activity setup
- XML layout resources
- String resources and localization
- Build variants (debug/release)
- Asset management
- ProGuard/R8 configuration
- APK and AAB generation

**Key Features**:
- Single module application
- Material Design components
- XML-based UI
- Navigation between activities
- Resource qualifiers (language, density, orientation)
- Basic instrumentation tests

**Build Command**:
```bash
cd basic-xml-app
horcrux build //app:debug
horcrux build //app:release
```

### 2. Compose App

**Location**: `compose-app/`

A modern Android application using Jetpack Compose, demonstrating:

- Compose UI development
- Compose compiler plugin integration
- Material 3 theming
- State management
- Compose navigation
- Compose metrics and reports
- Animation support

**Key Features**:
- 100% Compose UI
- Material 3 Design
- ViewModel integration
- Navigation with type safety
- Preview support
- Compose compiler optimizations
- Live literals (development)

**Build Command**:
```bash
cd compose-app
horcrux build //app:composeDebug --enable-compose-metrics
horcrux build //app:composeRelease
```

### 3. NDK App

**Location**: `ndk-app/`

An Android application with native C++ code, demonstrating:

- JNI (Java Native Interface) integration
- Multi-ABI builds (arm64-v8a, armeabi-v7a, x86, x86_64)
- NDK toolchain management
- Native library packaging
- C++ standard library selection
- Cross-ABI testing
- Hermetic NDK builds

**Key Features**:
- C++ business logic
- JNI bindings
- Mathematical utilities in native code
- Image processing with native code
- Multi-threaded native operations
- Native library versioning
- Symbol stripping and optimization

**Build Command**:
```bash
cd ndk-app
horcrux build //app:ndk-debug --all-abis
horcrux build //app:ndk-release --abi arm64-v8a
```

### 4. Flavor App

**Location**: `flavor-app/`

An application with multiple product flavors, demonstrating:

- Product flavors (free, paid, demo)
- Flavor dimensions (tier, environment)
- Build type variants (debug, release, staging)
- Variant-specific resources
- Flavor-specific dependencies
- Application ID suffixes
- Version code/name variants

**Key Features**:
- 3 product flavors: free, paid, enterprise
- 2 flavor dimensions: tier and api-level
- Variant matrix: 12 total variants
- Per-variant configuration
- Source set overlays
- Manifest merging per variant
- Conditional dependencies

**Build Command**:
```bash
cd flavor-app
# Build specific variant
horcrux build //app:freeDebug

# Build all variants
horcrux build //app:all-variants

# List all variants
horcrux query //app:variants
```

### 5. Multi-Module App

**Location**: `multi-module-app/`

A complex multi-module Android project, demonstrating:

- Module dependencies
- Feature modules
- Library modules
- Shared modules
- Circular dependency prevention
- Inter-module API boundaries
- Incremental compilation across modules
- Module-level caching

**Project Structure**:
```
multi-module-app/
├── app/                    # Main application module
├── features/
│   ├── feature-home/       # Home feature module
│   ├── feature-profile/    # Profile feature module
│   └── feature-settings/   # Settings feature module
├── libraries/
│   ├── lib-network/        # Network library
│   ├── lib-database/       # Database library
│   └── lib-analytics/      # Analytics library
└── shared/
    ├── shared-ui/          # Shared UI components
    ├── shared-models/      # Shared data models
    └── shared-utils/       # Shared utilities
```

**Key Features**:
- Clear module boundaries
- Dependency injection across modules
- Feature module isolation
- Shared resource management
- Per-module ProGuard rules
- Module-specific build configurations
- Dynamic feature module support (future)

**Build Command**:
```bash
cd multi-module-app
# Build entire app
horcrux build //app:release

# Build specific module
horcrux build //libraries/lib-network:lib

# Build and run tests
horcrux test //...
```

## CI Integration

The examples repository includes comprehensive CI workflows that serve as regression tests for Horcrux's Android build system.

### Build All Examples Workflow

**File**: `.github/workflows/build-all-examples.yml`

Builds all example projects on every commit to ensure:
- All examples compile successfully
- No API breakage
- Cross-platform compatibility (Linux, macOS)
- Multiple NDK versions support

### Regression Tests Workflow

**File**: `.github/workflows/regression-tests.yml`

Runs comprehensive tests including:
- Full clean builds
- Incremental builds
- Cache verification
- Multi-ABI verification
- Variant matrix expansion
- APK/AAB generation
- Build reproducibility checks
- Performance benchmarks

**Triggered by**:
- Pull requests to main Horcrux repository
- Scheduled daily runs
- Manual workflow dispatch

## Usage in Horcrux CI

The main Horcrux repository's CI can trigger builds in the examples repository to validate changes:

```yaml
# In horcrux/.github/workflows/android-integration.yml
jobs:
  trigger-examples:
    runs-on: ubuntu-latest
    steps:
      - name: Trigger Android Examples Build
        uses: actions/github-script@v7
        with:
          github-token: ${{ secrets.EXAMPLES_REPO_TOKEN }}
          script: |
            await github.rest.actions.createWorkflowDispatch({
              owner: 'horcruxsys',
              repo: 'horcrux-android-examples',
              workflow_id: 'regression-tests.yml',
              ref: 'main',
              inputs: {
                horcrux_version: context.sha
              }
            });
```

## Getting Started

### Prerequisites

1. Install Horcrux build system (latest version)
2. Set up Android SDK and NDK:
   ```bash
   export ANDROID_HOME=/path/to/android-sdk
   export ANDROID_NDK_ROOT=/path/to/android-ndk
   ```
3. Verify toolchain:
   ```bash
   horcrux doctor android
   ```

### Clone and Build

```bash
# Clone the examples repository
git clone https://github.com/horcruxsys/horcrux-android-examples.git
cd horcrux-android-examples

# Build all examples
./scripts/build-all.sh

# Or build individual examples
cd basic-xml-app
horcrux build //app:debug
```

### Running Examples on Device/Emulator

```bash
# Build and install APK
cd basic-xml-app
horcrux build //app:debug
adb install -r build/outputs/apk/debug/app-debug.apk

# Or use Horcrux's install command
horcrux install //app:debug
```

## Development Workflow

### Adding a New Example

1. Create a new directory for the example
2. Add a comprehensive `README.md` explaining:
   - What the example demonstrates
   - Key features and patterns
   - Build instructions
   - Expected output
3. Create `horcrux.yaml` configuration
4. Implement the example application
5. Add unit and instrumentation tests
6. Update the main repository README
7. Add CI workflow integration

### Testing Changes Locally

```bash
# Test all examples
./scripts/test-all.sh

# Test specific example
cd compose-app
horcrux test //...

# Verify CI locally
./scripts/verify-ci.sh
```

## Migration from Gradle

Each example includes a parallel Gradle build configuration (in a `gradle-reference/` subdirectory) to help with:

- Comparing Gradle and Horcrux configurations
- Validating build equivalence
- Understanding migration patterns
- Debugging discrepancies

See `docs/migration-guide.md` in the examples repository for detailed migration instructions.

## Best Practices

### Example Design Principles

1. **Production-Ready**: Examples should be real applications, not toys
2. **Well-Documented**: Comprehensive READMEs and inline comments
3. **Testable**: Include unit and instrumentation tests
4. **Maintainable**: Follow Android and Horcrux best practices
5. **Minimal Dependencies**: Only include necessary libraries
6. **Up-to-Date**: Keep dependencies and SDK versions current

### Build Configuration

1. **Explicit Dependencies**: Always specify exact versions
2. **Hermetic Builds**: No implicit system dependencies
3. **Reproducible**: Same inputs produce identical outputs
4. **Cached**: Leverage Horcrux's caching for fast rebuilds
5. **Documented**: Explain non-obvious configuration choices

## Contributing to Examples

Contributions to the examples repository are welcome! See the repository's `CONTRIBUTING.md` for guidelines on:

- Adding new examples
- Improving existing examples
- Fixing bugs
- Updating documentation
- Adding tests

## Troubleshooting

### Build Failures

If an example fails to build:

1. **Check toolchain**: Run `horcrux doctor android`
2. **Verify versions**: Ensure Android SDK/NDK versions match requirements
3. **Clean build**: Try `horcrux clean` and rebuild
4. **Check logs**: Review build logs for specific errors
5. **Compare with Gradle**: Check `gradle-reference/` for expected behavior

### CI Failures

If CI tests fail:

1. **Review CI logs**: Check GitHub Actions logs for details
2. **Run locally**: Use `./scripts/verify-ci.sh` to reproduce
3. **Check versions**: Ensure Horcrux version matches CI
4. **Report issues**: Open an issue in the main Horcrux repository

## Resources

- **Main Repository**: [github.com/horcruxsys/horcrux](https://github.com/horcruxsys/horcrux)
- **Examples Repository**: [github.com/horcruxsys/horcrux-android-examples](https://github.com/horcruxsys/horcrux-android-examples)
- **Documentation**: [horcruxsys.github.io/horcrux](https://horcruxsys.github.io/horcrux)
- **Issues**: [github.com/horcruxsys/horcrux/issues](https://github.com/horcruxsys/horcrux/issues)
- **Discussions**: [github.com/horcruxsys/horcrux/discussions](https://github.com/horcruxsys/horcrux/discussions)

## Version Compatibility

| Horcrux Version | Examples Version | Android Gradle Plugin | Kotlin Version | Compose Version |
|-----------------|------------------|----------------------|----------------|-----------------|
| 0.1.x           | 0.1.x            | 8.2.x                | 1.9.x          | 1.5.x           |
| 0.2.x (future)  | 0.2.x            | 8.3.x                | 1.9.x          | 1.6.x           |

## License

All examples are licensed under the MIT License, same as the main Horcrux project.

Copyright © 2025 Horcrux Project Contributors

---

**Note**: The `horcrux-android-examples` repository is maintained separately from the main Horcrux repository but is tightly integrated with the CI/CD pipeline to ensure continuous validation of Android build capabilities.
