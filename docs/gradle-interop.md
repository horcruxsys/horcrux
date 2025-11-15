# Gradle Interoperability Guide

This guide explains how to use Horcrux's Gradle interoperability layer to import and build existing Gradle-based projects with minimal friction.

## Overview

Horcrux provides seamless integration with Gradle-based projects, particularly Android projects. The `horcrux import` command analyzes your existing Gradle configuration and automatically generates a `horcrux.yaml` workspace configuration that maps your Gradle build to Horcrux's build system.

## Features

- ✅ **Automatic Configuration Generation**: Parse Gradle files and generate horcrux.yaml automatically
- ✅ **Multi-Format Support**: Works with both Groovy (.gradle) and Kotlin DSL (.gradle.kts) files
- ✅ **Dependency Mapping**: Extracts and maps Gradle dependencies to Horcrux format
- ✅ **Source Set Detection**: Automatically identifies source directories (Java, Kotlin, resources)
- ✅ **Build Variant Support**: Recognizes debug and release build configurations
- ✅ **Android Project Support**: Full support for Android application and library projects
- ✅ **Java/Kotlin Projects**: Works with pure Java and Kotlin projects too

## Quick Start

### Import a Gradle Project

Navigate to your Gradle project directory and run:

```bash
horcrux import .
```

This will:
1. Scan for `settings.gradle` or `settings.gradle.kts`
2. Parse `build.gradle` or `build.gradle.kts`
3. Extract project configuration, dependencies, and source sets
4. Generate `horcrux.yaml` in the current directory

### Import with Custom Output Location

Specify a custom output path for the generated configuration:

```bash
horcrux import /path/to/project --output=custom-config.yaml
# or use the short form
horcrux import /path/to/project -o custom-config.yaml
```

### Verbose Output

Enable verbose logging to see detailed information about the import process:

```bash
horcrux import . --verbose
```

## Supported Gradle Project Types

### Android Application

**Example `build.gradle`:**
```groovy
plugins {
    id 'com.android.application'
}

android {
    compileSdk 34
    
    defaultConfig {
        applicationId "com.example.myapp"
        minSdk 21
        targetSdk 34
        versionCode 1
        versionName "1.0"
    }
}

dependencies {
    implementation 'androidx.core:core-ktx:1.12.0'
    implementation 'androidx.appcompat:appcompat:1.6.1'
}
```

**Generated `horcrux.yaml`:**
```yaml
workspace:
  name: myapp
  version: "0.1.0"
  type: android-app

android:
  compileSdk: 34
  minSdk: 21
  targetSdk: 34
  applicationId: com.example.myapp
  versionName: 1.0
  versionCode: 1

sourceSets:
  main:
    java:
      - src/main/java
    kotlin:
      - src/main/kotlin
    resources:
      - src/main/res

dependencies:
  - name: core-ktx
    group: androidx.core
    version: 1.12.0
    scope: implementation
  - name: appcompat
    group: androidx.appcompat
    version: 1.6.1
    scope: implementation

targets:
  app:
    rule: android_app
    srcs:
      - src/main/java/**/*.java
      - src/main/kotlin/**/*.kt
    deps:
      - "androidx.core:core-ktx:1.12.0"
      - "androidx.appcompat:appcompat:1.6.1"
```

### Android Library

**Example `build.gradle`:**
```groovy
plugins {
    id 'com.android.library'
}

android {
    compileSdk 34
}

dependencies {
    api 'com.google.android.material:material:1.9.0'
}
```

**Generated Configuration:**
- `type: android-library`
- `rule: android_library`
- All dependencies mapped appropriately

### Kotlin DSL Projects

Horcrux fully supports Kotlin DSL (`.gradle.kts`) files:

**Example `build.gradle.kts`:**
```kotlin
plugins {
    id("com.android.application")
}

android {
    compileSdk = 34
    
    defaultConfig {
        applicationId = "com.example.kotlinapp"
        minSdk = 24
        targetSdk = 34
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
}
```

The import process handles both Groovy and Kotlin DSL syntax transparently.

### Java/Kotlin Applications

Non-Android projects are also supported:

**Example `build.gradle`:**
```groovy
plugins {
    id 'application'
}

dependencies {
    implementation 'com.google.guava:guava:32.1.3-jre'
}
```

Generated configuration will use `type: application` and `rule: java_application`.

## What Gets Extracted

### From `settings.gradle`

- **Root project name**: Used as workspace name
- **Subprojects**: Multi-module project structure

### From `build.gradle`

- **Project type**: application, library, android-app, android-library
- **Android configuration**: compileSdk, minSdk, targetSdk, applicationId, versionName, versionCode
- **Dependencies**: All dependency declarations with their scopes (implementation, api, testImplementation, etc.)
- **Source sets**: Default source directories (src/main/java, src/main/kotlin, src/main/res, etc.)
- **Build variants**: debug and release configurations

## Command Reference

### `horcrux import`

Import a Gradle project and generate horcrux.yaml configuration.

**Usage:**
```bash
horcrux import <project-path> [options]
```

**Arguments:**
- `<project-path>`: Path to the Gradle project directory (required)

**Options:**
- `--output=FILE`, `-o FILE`: Specify output file path (default: `horcrux.yaml` in project directory)
- `--verbose`, `-v`: Enable verbose logging
- `--help`: Show help message

**Examples:**
```bash
# Import current directory
horcrux import .

# Import specific project
horcrux import /path/to/android/project

# Custom output location
horcrux import . -o my-config.yaml

# Verbose mode
horcrux import . --verbose
```

## Troubleshooting

### "No Gradle files found in project"

**Cause:** The specified directory doesn't contain `build.gradle` or `build.gradle.kts`.

**Solution:** Ensure you're pointing to a valid Gradle project directory with a build file.

### "Failed to parse Gradle files"

**Cause:** The Gradle build file contains syntax that couldn't be parsed.

**Solution:** 
- Check that your build.gradle file is valid
- Some complex Gradle syntax may not be fully supported yet
- Try simplifying the build file or manually editing the generated horcrux.yaml

### Missing Dependencies

**Cause:** Some dependencies may not be detected if they use non-standard syntax.

**Solution:** Manually add missing dependencies to the generated `horcrux.yaml` file.

## Limitations

Current limitations of the Gradle import feature:

1. **Complex Build Logic**: Custom tasks and complex build logic are not automatically translated
2. **Build Script Plugins**: Plugins defined in `buildscript {}` blocks require manual configuration
3. **Multi-Module Projects**: Subprojects are listed but require individual import
4. **Custom Source Sets**: Non-standard source sets may need manual definition
5. **Dynamic Configuration**: Properties computed at runtime cannot be extracted

For unsupported features, manually edit the generated `horcrux.yaml` file.

## Best Practices

1. **Review Generated Config**: Always review the generated `horcrux.yaml` before using it in production
2. **Version Control**: Commit the generated `horcrux.yaml` to version control
3. **Incremental Migration**: Start with a single module in multi-module projects
4. **Test Thoroughly**: After import, test that builds work as expected with Horcrux
5. **Keep Gradle Files**: Maintain your Gradle files for compatibility with other tools

## Integration with Existing Workflows

### Gradual Migration

You can use both Gradle and Horcrux in the same project:

1. Import your project: `horcrux import .`
2. Use Horcrux for builds: `horcrux build //app:target`
3. Keep Gradle files for IDE support and other tools
4. Incrementally migrate build logic to Horcrux

### CI/CD Integration

```bash
# In your CI pipeline
- name: Import Gradle project
  run: horcrux import .

- name: Build with Horcrux
  run: horcrux build //app:all
```

## Examples

### Example 1: Simple Android App

```bash
$ cd my-android-app
$ horcrux import .
[INFO] Importing Gradle project from: /home/user/my-android-app
[INFO] Parsing settings file: settings.gradle
[INFO] Project name: MyAndroidApp
[INFO] Parsing build file: build.gradle
[INFO] Project type: android-app
[INFO] Found 5 dependencies
[INFO] Found 2 source sets
[INFO] Generating horcrux.yaml...
[INFO] Successfully generated: /home/user/my-android-app/horcrux.yaml
```

### Example 2: Multi-Module Project

```bash
$ cd my-project
$ horcrux import app/
[INFO] Importing Gradle project from: /home/user/my-project/app
[INFO] No settings.gradle found, using directory name: app
[INFO] Parsing build file: app/build.gradle
[INFO] Project type: android-app
[INFO] Successfully generated: /home/user/my-project/app/horcrux.yaml
```

### Example 3: Kotlin DSL Project

```bash
$ cd kotlin-project
$ horcrux import . --verbose
[DEBUG] Scanning for Gradle files...
[INFO] Importing Gradle project from: /home/user/kotlin-project
[INFO] Parsing settings file: settings.gradle.kts
[INFO] Project name: KotlinProject
[INFO] Parsing build file: build.gradle.kts
[DEBUG] Detected Kotlin DSL file
[INFO] Project type: android-app
[INFO] Found 3 dependencies
[INFO] Successfully generated: /home/user/kotlin-project/horcrux.yaml
```

## Next Steps

After importing your project:

1. **Review the Configuration**: Open `horcrux.yaml` and verify the generated configuration
2. **Build Your Project**: Try building with `horcrux build //app:target`
3. **Customize as Needed**: Edit `horcrux.yaml` to add custom build rules or configurations
4. **Explore Horcrux Features**: Take advantage of Horcrux's caching, parallel builds, and other features

## Getting Help

- **Documentation**: See the main Horcrux documentation
- **Issues**: Report problems at https://github.com/horcruxsys/horcrux/issues
- **Discussions**: Ask questions at https://github.com/horcruxsys/horcrux/discussions

## Contributing

Help improve Gradle interoperability:

- Report issues with Gradle file parsing
- Submit PRs to enhance import functionality
- Share your use cases and requirements

See [CONTRIBUTING.md](../CONTRIBUTING.md) for contribution guidelines.
