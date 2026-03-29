# Compose App

A modern Android application built entirely with [Jetpack Compose](https://developer.android.com/jetpack/compose), using Horcrux as the build system.

## Overview

This example demonstrates:
- Jetpack Compose UI with Material 3
- Compose Navigation between screens
- ViewModel integration with Compose
- Dark/light theme support
- Compose compiler metrics and reports

## Project Structure

```
compose-app/
├── BUILD                             # Horcrux build targets
├── horcrux.yaml                      # Workspace configuration
├── proguard-rules.pro                # ProGuard/R8 rules for release
├── src/
│   └── main/
│       ├── AndroidManifest.xml
│       └── java/com/example/compose/
│           ├── MainActivity.kt
│           ├── AppNavGraph.kt
│           └── ui/
│               ├── screens/
│               │   ├── HomeScreen.kt
│               │   └── DetailScreen.kt
│               └── theme/
│                   └── Theme.kt
└── README.md
```

## Building

```bash
# Debug APK (with Compose metrics and reports)
horcrux build //examples/android/compose-app:debug

# Release APK
horcrux build //examples/android/compose-app:release

# Run all tests
horcrux test //examples/android/compose-app/...
```

## Prerequisites

- Android SDK with API 34
- Kotlin 1.9.22
- Java 17+
- Horcrux build system

## Key Concepts Demonstrated

- `android_binary` rule with `kotlin_options` and `compose_options`
- Enabling `compose_metrics` and `compose_reports` in `android_apk`
- `horcrux.yaml` with `compose` configuration section
- Kotlin source file globbing
