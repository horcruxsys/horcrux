# Basic XML App

A traditional Android application using XML layouts, demonstrating how to use Horcrux as the build system for a standard Android project.

## Overview

This example shows:
- Activity setup with XML layouts
- Navigation between activities via `Intent`
- String and color resources
- Material Design theming
- Debug and release APK targets with ProGuard

## Project Structure

```
basic-xml-app/
├── BUILD                             # Horcrux build targets
├── horcrux.yaml                      # Workspace configuration
├── proguard-rules.pro                # ProGuard/R8 rules for release
├── src/
│   └── main/
│       ├── AndroidManifest.xml
│       ├── java/com/example/basicxml/
│       │   ├── MainActivity.java
│       │   └── DetailActivity.java
│       └── res/
│           ├── layout/
│           │   ├── activity_main.xml
│           │   └── activity_detail.xml
│           └── values/
│               ├── strings.xml
│               ├── colors.xml
│               └── themes.xml
└── README.md
```

## Building

```bash
# Debug APK
horcrux build //examples/android/basic-xml-app:debug

# Release APK
horcrux build //examples/android/basic-xml-app:release

# Run all tests
horcrux test //examples/android/basic-xml-app/...
```

## Prerequisites

- Android SDK with API 34
- Java 17+
- Horcrux build system

Set up the Android SDK path:
```bash
export ANDROID_HOME=/path/to/android-sdk
horcrux doctor android
```

## Key Concepts Demonstrated

- `android_binary` rule for building the app
- `android_apk` rule for packaging debug and release APKs
- Resource file globbing with `glob()`
- External Maven dependency references via `@maven//`
- `horcrux.yaml` workspace configuration for Android
