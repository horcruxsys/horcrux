# Multi-Flavor App

An Android application with product flavors (`free` and `paid`) and build types (`debug` and `release`), demonstrating Horcrux's multi-variant build support.

## Overview

This example demonstrates:
- Product flavor dimensions (`tier`: free, paid)
- Build type variants (debug, release)
- Variant matrix: 4 total variants (freeDebug, freeRelease, paidDebug, paidRelease)
- Per-flavor source sets and resource overlays
- Per-variant application IDs and version codes
- Flavor-specific strings and feature flags

## Project Structure

```
multi-flavor-app/
├── BUILD                             # Horcrux build targets (one per variant)
├── horcrux.yaml                      # Workspace with flavor/buildType config
├── proguard-rules.pro
├── src/
│   ├── main/                         # Shared base sources
│   │   ├── AndroidManifest.xml
│   │   ├── java/com/example/flavor/
│   │   │   └── MainActivity.java
│   │   └── res/
│   ├── free/                         # Free flavor overlay
│   │   ├── java/com/example/flavor/
│   │   │   └── FreeMainActivity.java
│   │   └── res/values/strings.xml
│   └── paid/                         # Paid flavor overlay
│       ├── java/com/example/flavor/
│       │   └── PaidMainActivity.java
│       └── res/values/strings.xml
└── README.md
```

## Building

```bash
# Build a specific variant
horcrux build //examples/android/multi-flavor-app:freeDebug
horcrux build //examples/android/multi-flavor-app:freeRelease
horcrux build //examples/android/multi-flavor-app:paidDebug
horcrux build //examples/android/multi-flavor-app:paidRelease

# List all available variants
horcrux query //examples/android/multi-flavor-app:variants

# Run unit tests
horcrux test //examples/android/multi-flavor-app/...
```

## Prerequisites

- Android SDK with API 34
- Java 17+
- Horcrux build system

## Key Concepts Demonstrated

- `android_library` for shared base and flavor-specific overlays
- `android_binary` with `flavor` and `build_type` attributes per variant
- `horcrux.yaml` `productFlavors` and `flavorDimensions` configuration
- Source set overlays via `src/free/` and `src/paid/` directories
- Per-variant `application_id` and `version_code`
