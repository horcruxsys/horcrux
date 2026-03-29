# Multi-Module App

A complex multi-module Android project demonstrating Horcrux's support for modular Android builds with clear module boundaries.

## Overview

This example demonstrates:
- Modular project structure with feature and library modules
- Inter-module Horcrux dependencies via `//path:target` labels
- Incremental compilation across modules
- Per-module BUILD files
- Shared utility and network library modules
- Feature module isolation

## Project Structure

```
multi-module-app/
├── BUILD                             # Root build: assembles full app
├── horcrux.yaml                      # Workspace with module list
├── app/                              # Main application module
│   ├── src/main/
│   │   ├── AndroidManifest.xml
│   │   ├── java/com/example/multimodule/
│   │   │   └── MainActivity.java
│   │   └── res/
│   └── proguard-rules.pro
├── features/
│   └── home/                         # Home feature module
│       ├── BUILD
│       └── src/main/
│           ├── AndroidManifest.xml
│           ├── java/com/example/home/
│           │   └── HomeFragment.java
│           └── res/
└── libraries/
    ├── network/                      # Network library module
    │   ├── BUILD
    │   └── src/main/java/com/example/network/
    │       └── HttpClient.java
    └── utils/                        # Utilities library module
        ├── BUILD
        └── src/main/java/com/example/utils/
            └── StringUtils.java
```

## Building

```bash
# Build the full app (debug)
horcrux build //examples/android/multi-module-app:debug

# Build the full app (release)
horcrux build //examples/android/multi-module-app:release

# Build only the network library
horcrux build //examples/android/multi-module-app/libraries/network:network

# Build only the home feature
horcrux build //examples/android/multi-module-app/features/home:home

# Build and test all modules
horcrux test //examples/android/multi-module-app/...
```

## Prerequisites

- Android SDK with API 34
- Java 17+
- Horcrux build system

## Key Concepts Demonstrated

- `android_library` for feature and library modules
- `android_binary` aggregating module deps via `//path:target` labels
- `android_apk` for packaging the assembled app
- Root `BUILD` file orchestrating the full build
- `horcrux.yaml` with `modules` list for workspace configuration
- Dependency chain: `app` → `home` → `network` + `utils`
