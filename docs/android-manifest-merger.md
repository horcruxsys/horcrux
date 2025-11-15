# Android Manifest Merger

This document describes Horcrux's Android Manifest Merger implementation, which provides deterministic and Gradle-compatible manifest merging for Android application development.

## Overview

The Android Manifest Merger combines multiple AndroidManifest.xml files from the main application and library dependencies into a single, unified manifest file. The merger follows **Gradle's manifest merger specification** to ensure compatibility with existing Android projects.

## Features

- ✅ **Gradle-Compatible**: Follows Android Gradle Plugin's merge rules
- ✅ **Priority-Based Merging**: Main > Flavor > BuildType > Library
- ✅ **Deterministic Output**: Same inputs always produce identical outputs
- ✅ **Element Matching**: By name, name+attribute, or android:name
- ✅ **Merge Actions**: merge, replace, merge-only, remove, strict
- ✅ **Conflict Resolution**: Configurable strategies for attribute conflicts
- ✅ **tools:node Support**: Explicit merge directives
- ✅ **Validation**: Checks for required manifest structure
- ✅ **Custom Rules**: Extensible merge rules for custom elements

## Quick Start

### Basic Usage

```cpp
#include "android_resources.h"

// Using AndroidResourceProcessor
AndroidResourceProcessor processor(toolchain);

ManifestMergeConfig config;
config.main_manifest = "src/main/AndroidManifest.xml";
config.library_manifests.push_back("libs/lib1/AndroidManifest.xml");
config.library_manifests.push_back("libs/lib2/AndroidManifest.xml");
config.output_manifest = "build/merged/AndroidManifest.xml";

auto result = processor.merge_manifests(config);
if (result) {
    std::cout << "Merged manifest: " << result->merged_manifest << std::endl;
}
```

### Advanced Usage

```cpp
#include "android_manifest_merger.h"

AndroidManifestMerger merger;

AndroidManifestMerger::MergeConfig config;
config.main_manifest = "src/main/AndroidManifest.xml";
config.library_manifests = {"libs/lib1/AndroidManifest.xml", "libs/lib2/AndroidManifest.xml"};
config.flavor_manifests = {"src/pro/AndroidManifest.xml"};
config.build_type_manifests = {"src/debug/AndroidManifest.xml"};
config.output_manifest = "build/merged/AndroidManifest.xml";
config.verbose = true;
config.strict = false;

auto result = merger.merge(config);
if (result && result->success) {
    std::cout << "Merge successful!" << std::endl;
    for (const auto& warning : result->warnings) {
        std::cout << "Warning: " << warning << std::endl;
    }
}
```

## Merge Priority

Manifests are merged in priority order, with higher priority manifests overriding lower priority ones:

```
┌─────────────────────┐
│   Main Manifest     │  ← Highest Priority
├─────────────────────┤
│  Flavor Manifests   │
├─────────────────────┤
│ Build Type Manifests│
├─────────────────────┤
│ Library Manifests   │  ← Lowest Priority
└─────────────────────┘
```

### Priority Examples

**Library Manifest** (lowest priority):
```xml
<manifest package="com.example.library">
    <application>
        <activity android:name=".LibraryActivity" android:exported="false" />
    </application>
</manifest>
```

**Main Manifest** (highest priority):
```xml
<manifest package="com.example.app">
    <application>
        <activity android:name="com.example.library.LibraryActivity" android:exported="true" />
        <activity android:name=".MainActivity" />
    </application>
</manifest>
```

**Merged Result**:
```xml
<manifest package="com.example.app">
    <application>
        <activity android:name="com.example.library.LibraryActivity" android:exported="true" />
        <activity android:name=".MainActivity" />
    </application>
</manifest>
```

The main manifest's `android:exported="true"` wins over the library's `android:exported="false"`.

## Merge Actions

### Default Actions

Elements are merged using these default actions:

| Element | Action | Matching Strategy |
|---------|--------|-------------------|
| `<application>` | merge | By name |
| `<activity>`, `<service>`, `<receiver>`, `<provider>` | merge | By android:name |
| `<uses-permission>`, `<uses-feature>`, `<uses-library>` | merge | By android:name |
| `<intent-filter>` | merge | None (all merged) |
| `<meta-data>` | merge | By android:name |
| `<uses-sdk>` | merge | By name |

### Action Types

#### merge (Default)
Merges child elements and attributes. If an element exists in multiple manifests, their children and attributes are combined.

```xml
<!-- Library -->
<application android:icon="@drawable/lib_icon">
    <activity android:name=".LibActivity" />
</application>

<!-- Main -->
<application android:label="My App">
    <activity android:name=".MainActivity" />
</application>

<!-- Result -->
<application android:icon="@drawable/lib_icon" android:label="My App">
    <activity android:name=".LibActivity" />
    <activity android:name=".MainActivity" />
</application>
```

#### replace
Replaces the entire element from lower priority with higher priority.

```xml
<!-- Library -->
<activity android:name=".ConfigActivity" android:theme="@style/LibTheme" />

<!-- Main (with tools:node="replace") -->
<activity android:name=".ConfigActivity" android:theme="@style/AppTheme" tools:node="replace" />

<!-- Result -->
<activity android:name=".ConfigActivity" android:theme="@style/AppTheme" />
```

#### merge-only
Only merges if the element exists in the higher priority manifest.

#### remove
Removes an element from lower priority manifest.

```xml
<!-- Library -->
<activity android:name=".UnwantedActivity" />

<!-- Main -->
<activity android:name=".UnwantedActivity" tools:node="remove" />

<!-- Result: UnwantedActivity is removed -->
```

#### strict
Fails the build on any conflict.

## tools:node Directives

Control merge behavior explicitly using the `tools:node` attribute:

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:tools="http://schemas.android.com/tools">
    
    <!-- Always merge this element -->
    <activity android:name=".Activity1" tools:node="merge" />
    
    <!-- Replace lower priority version completely -->
    <activity android:name=".Activity2" tools:node="replace" />
    
    <!-- Only merge if in higher priority -->
    <activity android:name=".Activity3" tools:node="merge-only" />
    
    <!-- Remove this element -->
    <activity android:name=".Activity4" tools:node="remove" />
    
    <!-- Fail on any conflict -->
    <activity android:name=".Activity5" tools:node="strict" />
</manifest>
```

## Attribute Merging

### Conflict Resolution Strategies

When the same attribute has different values in multiple manifests:

#### UseHigherPriority (Default)
Use the value from the higher priority manifest.

```cpp
merger.set_conflict_strategy("android:minSdkVersion", ConflictStrategy::UseHigherPriority);
```

#### UseLowerPriority
Use the value from the lower priority manifest.

#### Fail
Fail the merge if there's a conflict.

#### Concatenate
Concatenate values with comma separator.

### SDK Version Attributes

SDK version attributes always use higher priority:

```xml
<!-- Library: minSdk=21 -->
<!-- Main: minSdk=24 -->
<!-- Result: minSdk=24 (main wins) -->
```

## Element Matching

Elements are matched using different strategies depending on their type:

### By Name Only
Elements like `<application>` match by tag name:

```xml
<!-- Only one <application> element in result -->
<application>...</application>
```

### By android:name
Components match by their android:name attribute:

```xml
<!-- These are the SAME activity -->
<activity android:name=".MainActivity" />
<activity android:name=".MainActivity" android:exported="true" />

<!-- These are DIFFERENT activities -->
<activity android:name=".Activity1" />
<activity android:name=".Activity2" />
```

### No Matching Key
Elements like `<intent-filter>` don't have unique keys, so all are merged:

```xml
<!-- Library -->
<intent-filter>
    <action android:name="android.intent.action.VIEW" />
</intent-filter>

<!-- Main -->
<intent-filter>
    <action android:name="android.intent.action.EDIT" />
</intent-filter>

<!-- Result: Both intent-filters are included -->
```

## Custom Merge Rules

Add custom rules for your own elements:

```cpp
AndroidManifestMerger merger;

// Custom element with replace action
MergeRule custom_rule{
    .element_name = "custom-config",
    .default_action = MergeAction::Replace,
    .key_type = NodeKey::NameAndAttr,
    .key_attribute = "custom:id"
};
merger.add_merge_rule(custom_rule);

// Custom conflict strategy
merger.set_conflict_strategy("custom:value", ConflictStrategy::Concatenate);
```

## Deterministic Merging

All merges are **deterministic and reproducible**:

### Element Sorting
Elements are automatically sorted:
1. By element name (alphabetically)
2. By android:name attribute (if present)

This ensures:
- Same inputs → Same output
- Reproducible builds
- Hermetic caching
- Consistent CI/CD builds

### Example

**Input** (unsorted):
```xml
<application>
    <activity android:name=".ZActivity" />
    <activity android:name=".AActivity" />
    <service android:name=".MyService" />
</application>
```

**Output** (sorted):
```xml
<application>
    <activity android:name=".AActivity" />
    <activity android:name=".ZActivity" />
    <service android:name=".MyService" />
</application>
```

## Validation

The merger validates the output manifest:

### Required Structure
- Root element must be `<manifest>`
- Must have `package` attribute
- Must contain `<application>` element

### Checking Results

```cpp
auto result = merger.merge(config);

if (result && result->success) {
    std::cout << "Merge successful!" << std::endl;
    
    // Check for warnings
    if (!result->warnings.empty()) {
        std::cout << "Warnings:" << std::endl;
        for (const auto& warning : result->warnings) {
            std::cout << "  - " << warning << std::endl;
        }
    }
} else {
    std::cerr << "Merge failed!" << std::endl;
    if (result) {
        for (const auto& error : result->errors) {
            std::cerr << "  - " << error << std::endl;
        }
    }
}
```

## Real-World Examples

### Example 1: App with Multiple Libraries

```cpp
AndroidManifestMerger::MergeConfig config;
config.main_manifest = "app/src/main/AndroidManifest.xml";

// Add library manifests
config.library_manifests = {
    "libs/analytics/AndroidManifest.xml",
    "libs/payment/AndroidManifest.xml",
    "libs/social/AndroidManifest.xml"
};

config.output_manifest = "build/merged/AndroidManifest.xml";

auto result = merger.merge(config);
```

### Example 2: Product Flavors

```cpp
AndroidManifestMerger::MergeConfig config;
config.main_manifest = "src/main/AndroidManifest.xml";
config.library_manifests = {"libs/core/AndroidManifest.xml"};

// Add flavor-specific manifest
config.flavor_manifests = {"src/pro/AndroidManifest.xml"};

// Add build-type-specific manifest
config.build_type_manifests = {"src/debug/AndroidManifest.xml"};

config.output_manifest = "build/pro/debug/AndroidManifest.xml";

auto result = merger.merge(config);
```

### Example 3: Permission Consolidation

```cpp
// Libraries declare permissions they need
// Main app gets all permissions merged automatically

// Library1: INTERNET, CAMERA
// Library2: INTERNET, LOCATION
// Result: INTERNET (deduplicated), CAMERA, LOCATION
```

## API Reference

### AndroidManifestMerger

Main class for manifest merging.

```cpp
class AndroidManifestMerger {
public:
    AndroidManifestMerger();
    
    auto merge(const MergeConfig& config)
        -> tl::expected<MergeResult, AndroidResourceError>;
    
    void add_merge_rule(const MergeRule& rule);
    void set_conflict_strategy(const std::string& attribute, ConflictStrategy strategy);
};
```

### MergeConfig

Configuration for merge operation.

```cpp
struct MergeConfig {
    std::filesystem::path main_manifest;
    std::vector<std::filesystem::path> library_manifests;
    std::vector<std::filesystem::path> flavor_manifests;
    std::vector<std::filesystem::path> build_type_manifests;
    std::filesystem::path output_manifest;
    bool verbose = false;
    bool strict = false;
};
```

### MergeResult

Result of merge operation.

```cpp
struct MergeResult {
    std::filesystem::path merged_manifest;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    bool success;
};
```

### MergeAction

```cpp
enum class MergeAction {
    Merge,      // Merge child elements and attributes
    Replace,    // Replace with higher priority
    MergeOnly,  // Only if in higher priority
    Remove,     // Remove element
    Strict      // Fail on conflict
};
```

### ConflictStrategy

```cpp
enum class ConflictStrategy {
    UseHigherPriority,  // Use value from higher priority
    UseLowerPriority,   // Use value from lower priority
    Fail,               // Fail on conflict
    Concatenate         // Concatenate values
};
```

## Performance

The manifest merger is designed for efficiency:

- **Fast XML parsing**: Uses TinyXML2 for high-performance parsing
- **Minimal memory**: Processes manifests incrementally
- **O(n log n) sorting**: Deterministic element ordering
- **Cacheable**: Same inputs produce identical outputs

Typical merge times:
- Small project (< 5 manifests): < 10ms
- Medium project (< 20 manifests): < 50ms
- Large project (< 100 manifests): < 200ms

## Best Practices

### 1. Use Explicit tools:node When Needed

```xml
<!-- Be explicit about replacements -->
<activity android:name=".ConfigActivity" tools:node="replace" />
```

### 2. Minimize Library Manifest Content

Libraries should declare only what they need:
- Permissions they require
- Components they provide
- Services they expose

### 3. Document Merge Conflicts

When using tools:node directives, add comments:

```xml
<!-- Replace library's default theme with app theme -->
<activity android:name=".LibActivity" android:theme="@style/AppTheme" tools:node="replace" />
```

### 4. Test With Real Manifests

Always test your merger configuration with actual manifests from your dependencies.

### 5. Enable Verbose Mode for Debugging

```cpp
config.verbose = true;  // See what's being merged
```

## Limitations

Current limitations (may be addressed in future versions):

- No placeholder replacement (${applicationId}, etc.)
- No namespace support (Android Gradle Plugin 7.0+)
- No automatic tools:node attribute removal
- No manifest merger report generation

## Troubleshooting

### Common Issues

#### Conflict on android:exported

**Problem**: Different libraries set different values for android:exported.

**Solution**: Use tools:node="replace" in main manifest:

```xml
<activity android:name=".LibActivity" android:exported="true" tools:node="replace" />
```

#### Duplicate Permissions

**Problem**: Same permission declared multiple times.

**Solution**: This is handled automatically - duplicates are deduplicated.

#### Missing Application Element

**Problem**: Validation fails with "Must contain <application> element".

**Solution**: Ensure main manifest has `<application>` tag:

```xml
<manifest>
    <application>
        <!-- Your app content -->
    </application>
</manifest>
```

## See Also

- [Android Resource Processing](android-resources.md)
- [Android Toolchain](android-toolchain.md)
- [Coding Standards](coding-standards.md)
- [Android Gradle Plugin - Manifest Merger](https://developer.android.com/studio/build/manifest-merge)
