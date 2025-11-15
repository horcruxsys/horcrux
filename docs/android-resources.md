# Android Resource Processing (AAPT2 Pipeline)

This document describes Horcrux's Android Resource Processing system, which provides hermetic and reproducible resource compilation for Android application development using AAPT2.

## Overview

Horcrux provides native support for processing Android resources through the AAPT2 (Android Asset Packaging Tool 2) pipeline. The resource processor ensures:

- **Hermetic builds**: All resources are explicitly tracked and hashed
- **Incremental compilation**: Only changed resources are reprocessed
- **Deterministic builds**: Same inputs always produce identical outputs
- **Comprehensive resource support**: Values, layouts, drawables, mipmaps, and more
- **Resource qualifiers**: Density, locale, API level, orientation, and other qualifiers
- **Content-addressable caching**: Build artifacts are cached by their SHA-256 hash

## Features

- ✅ AAPT2 compile: XML resources → flat binary files
- ✅ AAPT2 link: Flat files + manifest → resources.ap_ + R.jar
- ✅ Resource merging from multiple directories
- ✅ Manifest merging
- ✅ Density-specific resources (ldpi, mdpi, hdpi, xhdpi, xxhdpi, xxxhdpi)
- ✅ Locale-specific resources
- ✅ API level qualifiers (v21, v28, v34, etc.)
- ✅ Resource overlays and flavors
- ✅ Incremental compilation with content hashing
- ✅ Deterministic resource sorting

## Resource Types Supported

| Resource Type | Directory | Description |
|--------------|-----------|-------------|
| Values | `res/values/` | Strings, colors, dimensions, styles |
| Layout | `res/layout/` | UI layout XML files |
| Drawable | `res/drawable/` | Images, shapes, gradients |
| Mipmap | `res/mipmap/` | Launcher icons |
| Raw | `res/raw/` | Raw files (assets) |
| XML | `res/xml/` | Generic XML resources |
| Anim | `res/anim/` | View animations |
| Animator | `res/animator/` | Property animations |
| Color | `res/color/` | Color state lists |
| Menu | `res/menu/` | Menu definitions |

## Resource Qualifiers

Horcrux supports all standard Android resource qualifiers:

### Density Qualifiers
- `ldpi` (~120dpi)
- `mdpi` (~160dpi)
- `hdpi` (~240dpi)
- `xhdpi` (~320dpi)
- `xxhdpi` (~480dpi)
- `xxxhdpi` (~640dpi)
- `nodpi` (density-independent)
- `tvdpi` (~213dpi for TV)
- `anydpi` (any density)

### Locale Qualifiers
- Language: `en`, `es`, `fr`, `de`, etc.
- Region: `rUS`, `rGB`, `rES`, etc.
- Example: `values-en-rUS/strings.xml`

### API Level Qualifiers
- Format: `v<api-level>`
- Examples: `v21`, `v28`, `v34`
- Example: `values-v21/styles.xml`

### Other Qualifiers
- Screen size: `small`, `normal`, `large`, `xlarge`
- Orientation: `port` (portrait), `land` (landscape)
- Night mode: `night`, `notnight`

### Qualifier Combinations

Qualifiers can be combined following Android's resource qualifier precedence:

```
values-en-rUS-mdpi-v21/strings.xml
drawable-hdpi-night/icon.png
layout-land-xlarge/activity_main.xml
```

## Usage

### Basic AAPT2 Compilation

```cpp
#include "android_resources.h"
#include "android_toolchain.h"

using namespace horcrux::core;

// Detect Android toolchain
auto toolchain_result = AndroidToolchainDetector::detect();
if (!toolchain_result) {
    // Handle error
}

// Create resource processor
AndroidResourceProcessor processor(*toolchain_result);

// Verify AAPT2 is available
if (!processor.get_aapt2_path()) {
    std::cerr << "AAPT2 not found in build tools" << std::endl;
    return;
}

// Configure compilation
Aapt2CompileConfig config;
config.output_dir = "build/compiled_resources";
config.incremental = true;
config.verbose = false;

// Add resources
ResourceFile strings_resource;
strings_resource.path = "res/values/strings.xml";
strings_resource.type = ResourceType::Values;
strings_resource.content_hash = ResourceFile::compute_hash(strings_resource.path).value();
config.resources.push_back(std::move(strings_resource));

ResourceFile layout_resource;
layout_resource.path = "res/layout/activity_main.xml";
layout_resource.type = ResourceType::Layout;
layout_resource.content_hash = ResourceFile::compute_hash(layout_resource.path).value();
config.resources.push_back(std::move(layout_resource));

// Compile resources
auto result = processor.compile(config);
if (!result) {
    std::cerr << "Compilation failed: " << to_string(result.error()) << std::endl;
    return;
}

std::cout << "Compiled " << result->compiled_files.size() << " resources in "
          << result->compilation_time.count() << "ms" << std::endl;
```

### AAPT2 Linking

```cpp
// Configure linking
Aapt2LinkConfig link_config;
link_config.compiled_resources = result->compiled_files;
link_config.manifest = "AndroidManifest.xml";
link_config.output_apk = "build/resources.ap_";
link_config.r_java_output = "build/gen";
link_config.android_jar = toolchain_result->platforms[0].android_jar_path;
link_config.verbose = false;

// Link resources
auto link_result = processor.link(link_config);
if (!link_result) {
    std::cerr << "Linking failed: " << to_string(link_result.error()) << std::endl;
    return;
}

std::cout << "Generated resources.ap_ and R.jar in "
          << link_result->link_time.count() << "ms" << std::endl;
```

### Complete Pipeline

The complete pipeline handles merging, compilation, and linking in a single call:

```cpp
AndroidResourceProcessor::PipelineConfig pipeline_config;

// Add multiple resource directories (for overlays/flavors)
pipeline_config.resource_dirs.push_back("src/main/res");
pipeline_config.resource_dirs.push_back("src/debug/res");
pipeline_config.resource_dirs.push_back("src/flavor1/res");

pipeline_config.manifest = "src/main/AndroidManifest.xml";
pipeline_config.output_dir = "build/intermediates/resources";
pipeline_config.android_jar = toolchain_result->platforms[0].android_jar_path;
pipeline_config.package_name = "com.example.myapp";
pipeline_config.incremental = true;
pipeline_config.verbose = false;

// Process entire pipeline
auto pipeline_result = processor.process_pipeline(pipeline_config);
if (!pipeline_result) {
    std::cerr << "Pipeline failed: " << to_string(pipeline_result.error()) << std::endl;
    return;
}

std::cout << "Pipeline completed in " << pipeline_result->total_time.count() << "ms" << std::endl;
std::cout << "  R.jar: " << pipeline_result->r_jar << std::endl;
std::cout << "  resources.ap_: " << pipeline_result->resources_apk << std::endl;
```

## Resource Scanning

Automatically scan a directory tree for Android resources:

```cpp
auto scan_result = resource_utils::scan_resources("res");
if (!scan_result) {
    std::cerr << "Scan failed" << std::endl;
    return;
}

std::cout << "Found " << scan_result->size() << " resources" << std::endl;

// Sort resources deterministically
resource_utils::sort_resources(*scan_result);

// Filter by type
auto layouts = resource_utils::filter_by_type(*scan_result, ResourceType::Layout);
std::cout << "Found " << layouts.size() << " layout files" << std::endl;

// Filter by qualifiers
ResourceQualifiers hdpi_filter;
hdpi_filter.density = ResourceDensity::HDPI;
auto hdpi_resources = resource_utils::filter_by_qualifiers(*scan_result, hdpi_filter);
std::cout << "Found " << hdpi_resources.size() << " HDPI resources" << std::endl;
```

## Manifest Parsing

Parse and extract information from AndroidManifest.xml:

```cpp
auto manifest_result = AndroidManifest::parse("AndroidManifest.xml");
if (!manifest_result) {
    std::cerr << "Failed to parse manifest" << std::endl;
    return;
}

std::cout << "Package: " << manifest_result->package_name << std::endl;
std::cout << "Version: " << manifest_result->version_name 
          << " (code: " << manifest_result->version_code << ")" << std::endl;
std::cout << "Min SDK: " << manifest_result->min_sdk_version << std::endl;
std::cout << "Target SDK: " << manifest_result->target_sdk_version << std::endl;
```

## Resource Merging

Merge resources from multiple directories with deterministic ordering:

```cpp
ResourceMergeConfig merge_config;
merge_config.resource_dirs.push_back("src/main/res");
merge_config.resource_dirs.push_back("src/debug/res");
merge_config.resource_dirs.push_back("build/generated/res");
merge_config.output_dir = "build/merged_res";
merge_config.deterministic = true;
merge_config.verbose = false;

auto merge_result = processor.merge_resources(merge_config);
if (!merge_result) {
    std::cerr << "Merge failed" << std::endl;
    return;
}

std::cout << "Merged " << merge_result->merged_resources.size() 
          << " resources to " << merge_result->merged_dir << std::endl;
std::cout << "Merge hash: " << merge_result->merge_hash << std::endl;
```

## Manifest Merging

Horcrux implements **deterministic manifest merging** following Gradle's manifest merger specification. This ensures that manifests from the main app and all libraries are correctly merged with proper priority handling.

### Basic Manifest Merging

Merge multiple manifest files (main + libraries):

```cpp
ManifestMergeConfig manifest_config;
manifest_config.main_manifest = "src/main/AndroidManifest.xml";
manifest_config.library_manifests.push_back("libs/lib1/AndroidManifest.xml");
manifest_config.library_manifests.push_back("libs/lib2/AndroidManifest.xml");
manifest_config.output_manifest = "build/intermediates/merged_manifest/AndroidManifest.xml";
manifest_config.verbose = false;

auto manifest_result = processor.merge_manifests(manifest_config);
if (!manifest_result) {
    std::cerr << "Manifest merge failed" << std::endl;
    return;
}

std::cout << "Merged manifest: " << manifest_result->merged_manifest << std::endl;
```

### Advanced Manifest Merging

For more control over the merge process, use the `AndroidManifestMerger` class directly:

```cpp
#include "android_manifest_merger.h"

AndroidManifestMerger merger;

AndroidManifestMerger::MergeConfig config;
config.main_manifest = "src/main/AndroidManifest.xml";
config.library_manifests.push_back("libs/lib1/AndroidManifest.xml");
config.library_manifests.push_back("libs/lib2/AndroidManifest.xml");
config.flavor_manifests.push_back("src/flavor/AndroidManifest.xml");
config.build_type_manifests.push_back("src/debug/AndroidManifest.xml");
config.output_manifest = "build/merged/AndroidManifest.xml";
config.verbose = true;
config.strict = false; // Don't fail on conflicts

auto result = merger.merge(config);
if (!result || !result->success) {
    for (const auto& error : result->errors) {
        std::cerr << "Error: " << error << std::endl;
    }
    return;
}

// Check warnings
for (const auto& warning : result->warnings) {
    std::cout << "Warning: " << warning << std::endl;
}
```

### Manifest Merge Priority

Manifests are merged with the following priority order (highest to lowest):

1. **Main Manifest** - Highest priority
2. **Flavor Manifests** - Product flavor overlays
3. **Build Type Manifests** - Build type overlays (debug/release)
4. **Library Manifests** - Lowest priority

When conflicts occur, the higher priority manifest wins.

### Merge Rules

Horcrux supports different merge actions for different element types:

#### Merge Actions

- **merge** (default): Merge child elements and attributes
- **replace**: Replace lower-priority elements with higher-priority ones
- **merge-only**: Keep only if present in higher priority manifest
- **remove**: Remove element from final manifest
- **strict**: Fail on any conflict

#### Element Matching

Elements are matched using different strategies:

- **By Name**: Elements like `<application>` match by tag name only
- **By Name + Attribute**: Elements like `<activity>`, `<service>` match by `android:name`
- **By Name + ID**: Elements like `<uses-permission>` match by `android:name`

#### Supported Elements

The merger has built-in rules for common Android manifest elements:

- `<application>` - Merge children and attributes
- `<activity>`, `<service>`, `<receiver>`, `<provider>` - Match by `android:name`
- `<uses-permission>`, `<uses-feature>`, `<uses-library>` - Match by `android:name`
- `<intent-filter>` - Merge all (no unique key)
- `<meta-data>` - Match by `android:name`
- `<uses-sdk>` - Merge attributes

### Using tools:node Directives

You can control merge behavior using `tools:node` attributes:

```xml
<!-- In library manifest: Always include this activity -->
<activity
    android:name=".LibraryActivity"
    tools:node="merge" />

<!-- In main manifest: Replace library's default configuration -->
<activity
    android:name=".MainActivity"
    android:exported="true"
    tools:node="replace" />

<!-- Remove an element from a library manifest -->
<activity
    android:name=".UnwantedActivity"
    tools:node="remove" />
```

### Custom Merge Rules

Add custom merge rules for specific elements:

```cpp
AndroidManifestMerger merger;

// Add custom rule for a custom element
MergeRule custom_rule{
    .element_name = "custom-element",
    .default_action = MergeAction::Replace,
    .key_type = NodeKey::NameAndAttr,
    .key_attribute = "custom:id"
};
merger.add_merge_rule(custom_rule);

// Set conflict strategy for specific attributes
merger.set_conflict_strategy("android:minSdkVersion", ConflictStrategy::UseHigherPriority);
merger.set_conflict_strategy("android:label", ConflictStrategy::UseHigherPriority);
```

### Conflict Resolution Strategies

Control how attribute conflicts are resolved:

- **UseHigherPriority** (default): Use value from higher priority manifest
- **UseLowerPriority**: Use value from lower priority manifest
- **Fail**: Fail on conflict
- **Concatenate**: Concatenate values with comma separator

### Deterministic Merging

All manifest merging is **deterministic and reproducible**:

- Elements are sorted by name and key attributes
- Same inputs always produce identical outputs
- Merge order is consistent across builds
- Suitable for hermetic and cacheable builds

### Validation

The merger validates the final manifest:

```cpp
auto result = merger.merge(config);

if (result->warnings.empty() && result->errors.empty()) {
    std::cout << "Manifest merge successful with no issues" << std::endl;
} else {
    std::cout << "Warnings: " << result->warnings.size() << std::endl;
    std::cout << "Errors: " << result->errors.size() << std::endl;
}
```

Required checks:
- Root element must be `<manifest>`
- Must have `package` attribute
- Must contain `<application>` element

## Incremental Compilation

Horcrux uses SHA-256 content hashing for efficient incremental compilation:

### Computing Resource Hashes

```cpp
auto hash_result = ResourceFile::compute_hash("res/values/strings.xml");
if (hash_result) {
    std::cout << "Hash: " << *hash_result << std::endl;
}
```

### Checking for Changes

```cpp
auto current_hash = resource_incremental::compute_resource_hash("res/values/strings.xml");
auto cached_hash = "abc123..."; // Load from previous build

if (resource_incremental::has_resource_changed("res/values/strings.xml", cached_hash)) {
    std::cout << "Resource has changed, recompiling..." << std::endl;
} else {
    std::cout << "Resource unchanged, using cache" << std::endl;
}
```

### Saving Compilation State

```cpp
// After successful compilation
auto state_file = "build/.horcrux_resource_state";
auto compilation_hash = AndroidResourceProcessor::compute_compile_hash(config);

bool saved = resource_incremental::save_compilation_state(
    state_file, config, compilation_hash);
```

### Loading Compilation State

```cpp
auto state_file = "build/.horcrux_resource_state";
auto state = resource_incremental::load_compilation_state(state_file);

if (state) {
    std::cout << "Previous compilation: " << state->compilation_hash << std::endl;
    std::cout << "Tracked resources: " << state->resource_hashes.size() << std::endl;
    
    // Check if recompilation is needed
    auto current_hash = AndroidResourceProcessor::compute_compile_hash(config);
    if (current_hash == state->compilation_hash) {
        std::cout << "No changes, using cached compilation" << std::endl;
    }
}
```

## Error Handling

All resource processing functions use `tl::expected` for error handling:

```cpp
enum class AndroidResourceError {
  Aapt2NotFound,        // AAPT2 not found in build tools
  InvalidResourceFile,  // Resource file missing or invalid
  InvalidManifest,      // AndroidManifest.xml is invalid
  CompilationFailed,    // AAPT2 compile failed
  LinkingFailed,        // AAPT2 link failed
  MergingFailed,        // Resource merge failed
  InvalidConfiguration, // Invalid configuration
  IoError,              // I/O error
  UnknownError          // Unknown error
};
```

### Error Handling Pattern

```cpp
auto result = processor.compile(config);
if (!result) {
    // Handle error
    std::cerr << "Error: " << to_string(result.error()) << std::endl;
    
    switch (result.error()) {
        case AndroidResourceError::Aapt2NotFound:
            std::cerr << "Please install Android SDK build-tools" << std::endl;
            break;
        case AndroidResourceError::CompilationFailed:
            std::cerr << "Check AAPT2 output for details" << std::endl;
            break;
        case AndroidResourceError::InvalidResourceFile:
            std::cerr << "One or more resource files are invalid" << std::endl;
            break;
        // ... handle other errors
    }
    
    return;
}

// Use successful result
auto& compile_result = *result;
```

## Configuration Validation

Validate configuration before processing:

```cpp
// Validate compile configuration
auto validation = AndroidResourceProcessor::validate_compile_config(config);
if (!validation) {
    std::cerr << "Invalid compile config: " << to_string(validation.error()) << std::endl;
    return;
}

// Validate link configuration
auto link_validation = AndroidResourceProcessor::validate_link_config(link_config);
if (!link_validation) {
    std::cerr << "Invalid link config: " << to_string(link_validation.error()) << std::endl;
    return;
}
```

## Content-Addressable Storage

Resource processing integrates with Horcrux's CAS (Content-Addressable Storage) system:

```cpp
// Get compilation hash (Merkle signature)
std::string hash = result->compilation_hash;

// Store in CAS
LocalCache cache("~/.horcrux/cache");
for (const auto& compiled : result->compiled_files) {
    auto content = read_file(compiled);
    cache.store(hash + "/" + compiled.filename(), content);
}

// Later, retrieve from CAS
if (cache.contains(hash)) {
    auto artifacts = cache.lookup(hash);
    // Use cached artifacts
}
```

## Deterministic Builds

Resource processing ensures deterministic builds through:

### Resource Sorting

All resources are sorted before processing:

```cpp
// Sorting is done automatically in resource merging
auto resources = resource_utils::scan_resources("res");
resource_utils::sort_resources(*resources);

// Resources are sorted by:
// 1. Type (values, layout, drawable, etc.)
// 2. Qualifiers (locale, density, API level)
// 3. File path
```

### Hash Computation

All configuration elements are included in hash computation in deterministic order:

1. Source files (sorted by path)
2. Resource content hashes (sorted)
3. Configuration options (sorted)
4. Android platform JAR path
5. Additional arguments (sorted)

```cpp
// Compute compilation hash
auto compile_hash = AndroidResourceProcessor::compute_compile_hash(config);

// Compute link hash
auto link_hash = AndroidResourceProcessor::compute_link_hash(link_config);
```

## Integration with Android Toolchain

Resource processor integrates with the Android toolchain detection system:

```cpp
// Detect Android toolchain
auto toolchain = AndroidToolchainDetector::detect();
if (!toolchain) {
    std::cerr << "Android toolchain not found" << std::endl;
    return;
}

// Create processor
AndroidResourceProcessor processor(*toolchain);

// AAPT2 is automatically detected from build-tools
auto aapt2_path = processor.get_aapt2_path();
if (aapt2_path) {
    std::cout << "Using AAPT2: " << *aapt2_path << std::endl;
} else {
    std::cerr << "AAPT2 not found in build-tools" << std::endl;
}

// Use detected android.jar
if (!toolchain->platforms.empty()) {
    link_config.android_jar = toolchain->platforms[0].android_jar_path;
}
```

## Performance Considerations

### Incremental Compilation

- **First build**: All resources are compiled
- **Subsequent builds**: Only changed resources are recompiled
- **Cache hits**: Zero compilation time when nothing changed

### Parallel Processing

For multi-module projects, process resources in parallel:

```cpp
std::vector<std::future<Aapt2CompileResult>> futures;

for (const auto& module_config : module_configs) {
    futures.push_back(std::async(std::launch::async, [&]() {
        AndroidResourceProcessor processor(toolchain);
        return processor.compile(module_config);
    }));
}

// Wait for all compilations
for (auto& future : futures) {
    auto result = future.get();
    // Process result
}
```

## Best Practices

### 1. Always Enable Incremental Compilation

```cpp
config.incremental = true;  // Default, but be explicit
```

### 2. Use Deterministic Resource Merging

```cpp
merge_config.deterministic = true;  // Ensures reproducible builds
```

### 3. Validate Configurations

```cpp
auto validation = AndroidResourceProcessor::validate_compile_config(config);
if (!validation) {
    // Handle error before attempting compilation
}
```

### 4. Compute Hashes for All Resources

```cpp
for (auto& resource : config.resources) {
    auto hash_result = ResourceFile::compute_hash(resource.path);
    if (hash_result) {
        resource.content_hash = *hash_result;
    }
}
```

### 5. Save Compilation State

```cpp
// Save compilation state for incremental builds
if (config.incremental) {
    auto state_file = config.output_dir / ".horcrux_resource_state";
    resource_incremental::save_compilation_state(state_file, config, result->compilation_hash);
}
```

### 6. Use Resource Overlays

```cpp
// Order matters: later directories override earlier ones
pipeline_config.resource_dirs.push_back("src/main/res");      // Base resources
pipeline_config.resource_dirs.push_back("src/flavor1/res");   // Flavor overlay
pipeline_config.resource_dirs.push_back("src/debug/res");     // Build type overlay
```

## Testing

The resource processing system has comprehensive unit test coverage:

### Running Resource Tests

```bash
cd build
ctest --output-on-failure -R AndroidResource
```

### Test Categories

- **Error handling**: Error to string conversion, validation
- **Resource types**: Type detection and conversion
- **Qualifiers**: Parsing locale, density, API level
- **Hashing**: SHA-256 content hashing, determinism
- **Manifest parsing**: Package name, versions, SDK levels
- **Resource scanning**: Directory traversal, filtering
- **Incremental compilation**: State save/load, change detection
- **Configuration validation**: Compile and link config checks

## Future Enhancements

- [x] ~~Advanced manifest merger with library manifest merging~~ - **Implemented!**
- [ ] R.jar generation from R.java (currently placeholder)
- [ ] ProGuard rule generation during linking
- [ ] Resource shrinking and optimization
- [ ] Vector drawable optimization
- [ ] WebP conversion for images
- [ ] Remote resource caching
- [ ] Distributed resource compilation
- [ ] Resource obfuscation
- [ ] Namespace support for manifest merging (Android Gradle Plugin 7.0+)
- [ ] Placeholder replacement (${applicationId}, etc.)
- [ ] Manifest merger reports and conflict visualization

## See Also

- [Android Toolchain Detection](android-toolchain.md)
- [Android Java Compiler Rules](android-compiler-rules.md)
- [Android Manifest Merger](android-manifest-merger.md)
- [Coding Standards](coding-standards.md)
- [Architecture](architecture.md)
