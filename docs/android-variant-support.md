# Android Multi-Variant Build Support

This document describes Horcrux's Android multi-variant build support, which provides comprehensive handling of buildTypes, productFlavors, flavorDimensions, and variant matrix expansion.

## Overview

Horcrux provides native support for Android build variants, matching and exceeding Gradle's variant system capabilities. The variant system ensures:

- **Deterministic Builds**: Same inputs always produce identical variant outputs
- **Efficient Matrix Expansion**: Automatic generation of all variant combinations
- **Correct Configuration Merging**: Proper precedence and inheritance of variant properties
- **Hermetic Variant Isolation**: Each variant has its own isolated build environment
- **Content-Addressable Caching**: Variants are cached independently by their unique hashes

## Features

- ✅ **Build Types**: Support for debug, release, and custom build types
- ✅ **Product Flavors**: Multi-dimensional product flavors
- ✅ **Flavor Dimensions**: Cartesian product expansion across dimensions
- ✅ **Configuration Merging**: Application ID, version, SDK, and build flags
- ✅ **Source Set Merging**: Proper overlay of source, resource, and manifest files
- ✅ **Gradle Interop**: Parse existing Gradle files to extract variant configuration
- ✅ **Deterministic Naming**: Gradle-compatible variant naming (e.g., freeDebug, proRelease)
- ✅ **Variant Hashing**: SHA-256 hashing for incremental builds

## Quick Start

### Basic Usage with API

```cpp
#include "android_variant.h"

using namespace horcrux::core;

// Create variant matrix configuration
VariantMatrixConfig config;
config.base_application_id = "com.example.app";
config.base_version_code = 1;
config.base_version_name = "1.0";
config.base_min_sdk = 21;
config.base_target_sdk = 34;

// Add source directories
config.base_source_dirs.push_back("src/main/java");
config.base_source_dirs.push_back("src/main/kotlin");
config.base_resource_dirs.push_back("src/main/res");
config.base_manifest_files.push_back("src/main/AndroidManifest.xml");

// Define build types
BuildType debug;
debug.name = "debug";
debug.debuggable = true;
debug.minify_enabled = false;
debug.application_id_suffix = ".debug";
config.build_types.push_back(debug);

BuildType release;
release.name = "release";
release.debuggable = false;
release.minify_enabled = true;
release.shrink_resources = true;
config.build_types.push_back(release);

// Build the variant matrix
auto result = AndroidVariantBuilder::build_matrix(config);
if (!result) {
    std::cerr << "Error: " << result.error() << std::endl;
    return;
}

auto& matrix = *result;
std::cout << "Generated " << matrix.variants.size() << " variants\n";

for (const auto& variant : matrix.variants) {
    std::cout << "  - " << variant.name 
              << " (app ID: " << variant.application_id << ")\n";
}
```

### With Product Flavors

```cpp
// Define flavor dimensions
config.flavor_dimensions.push_back("tier");

// Define product flavors
ProductFlavor free;
free.name = "free";
free.dimension = "tier";
free.application_id_suffix = ".free";
config.flavors.push_back(free);

ProductFlavor pro;
pro.name = "pro";
pro.dimension = "tier";
pro.application_id_suffix = ".pro";
pro.version_code = 100;
config.flavors.push_back(pro);

// Build matrix
auto result = AndroidVariantBuilder::build_matrix(config);

// Result: 4 variants
// - freeDebug (com.example.app.free.debug)
// - freeRelease (com.example.app.free)
// - proDebug (com.example.app.pro.debug)
// - proRelease (com.example.app.pro)
```

### Multi-Dimensional Flavors

```cpp
// Define two dimensions
config.flavor_dimensions.push_back("tier");
config.flavor_dimensions.push_back("store");

// Tier flavors
ProductFlavor free;
free.name = "free";
free.dimension = "tier";
config.flavors.push_back(free);

ProductFlavor pro;
pro.name = "pro";
pro.dimension = "tier";
config.flavors.push_back(pro);

// Store flavors
ProductFlavor google;
google.name = "google";
google.dimension = "store";
config.flavors.push_back(google);

ProductFlavor amazon;
amazon.name = "amazon";
amazon.dimension = "store";
amazon.application_id_suffix = ".amazon";
config.flavors.push_back(amazon);

// Build matrix
auto result = AndroidVariantBuilder::build_matrix(config);

// Result: 8 variants (2 tiers × 2 stores × 2 build types)
// - freeGoogleDebug
// - freeGoogleRelease
// - freeAmazonDebug
// - freeAmazonRelease
// - proGoogleDebug
// - proGoogleRelease
// - proAmazonDebug
// - proAmazonRelease
```

## Configuration Reference

### BuildType

Represents a build type (debug, release, staging, etc.):

```cpp
struct BuildType {
  std::string name;                              // Build type name
  bool debuggable = false;                       // Enable debugging
  bool minify_enabled = false;                   // Enable code minification
  bool shrink_resources = false;                 // Enable resource shrinking
  std::optional<std::string> application_id_suffix;    // Suffix for app ID
  std::optional<std::string> version_name_suffix;      // Suffix for version name
  std::map<std::string, std::string> manifest_placeholders;  // Manifest placeholders
  std::vector<std::filesystem::path> source_dirs;      // Additional source dirs
  std::vector<std::filesystem::path> resource_dirs;    // Additional resource dirs
  std::vector<std::filesystem::path> manifest_files;   // Additional manifests
};
```

### ProductFlavor

Represents a product flavor:

```cpp
struct ProductFlavor {
  std::string name;                              // Flavor name
  std::string dimension;                         // Dimension this flavor belongs to
  std::optional<std::string> application_id;     // Override application ID
  std::optional<std::string> application_id_suffix;    // Suffix for app ID
  std::optional<int> version_code;               // Override version code
  std::optional<std::string> version_name;       // Override version name
  std::optional<int> min_sdk;                    // Override min SDK
  std::optional<int> target_sdk;                 // Override target SDK
  std::map<std::string, std::string> manifest_placeholders;  // Manifest placeholders
  std::vector<std::filesystem::path> source_dirs;      // Additional source dirs
  std::vector<std::filesystem::path> resource_dirs;    // Additional resource dirs
  std::vector<std::filesystem::path> manifest_files;   // Additional manifests
};
```

### BuildVariant

Represents a complete build variant:

```cpp
struct BuildVariant {
  std::string name;                              // Variant name (e.g., freeDebug)
  BuildType build_type;                          // Build type
  std::vector<ProductFlavor> flavors;            // Flavors (one per dimension)
  
  // Merged configuration
  std::string application_id;
  int version_code;
  std::string version_name;
  std::optional<int> min_sdk;
  std::optional<int> target_sdk;
  bool debuggable;
  bool minify_enabled;
  bool shrink_resources;
  
  // Merged paths
  std::vector<std::filesystem::path> source_dirs;
  std::vector<std::filesystem::path> resource_dirs;
  std::vector<std::filesystem::path> manifest_files;
  
  // Dependencies for this variant
  std::vector<std::string> dependencies;
  
  // Compute hash for this variant
  auto compute_hash() const -> std::string;
};
```

## Configuration Merging Rules

Horcrux follows Gradle's merge priority when combining variant configurations:

### Priority Order (Highest to Lowest)

1. **Build Type**: Properties from the build type
2. **Product Flavors**: Properties from flavors (in dimension order)
3. **Default/Main**: Base configuration

### Application ID Merging

```
Final Application ID = base_application_id + flavor_suffixes + build_type_suffix
```

Example:
```
Base: com.example.app
Flavor: .pro
Build Type: .debug
Result: com.example.app.pro.debug
```

### Version Name Merging

Flavors can override the version name entirely, or build types can add a suffix:

```
Final Version Name = flavor_version_name OR base_version_name + build_type_suffix
```

### Source Directory Merging

Source directories are merged in order:

```
Final Sources = base_sources + flavor_sources (in dimension order) + build_type_sources
```

Example:
```
Base: src/main/java, src/main/kotlin
Flavor (free): src/free/java
Build Type (debug): src/debug/java
Result: src/main/java, src/main/kotlin, src/free/java, src/debug/java
```

### SDK Version Merging

The first flavor to specify a min/target SDK wins:

```cpp
// First flavor with min_sdk specified
for (const auto& flavor : flavors) {
    if (flavor.min_sdk.has_value()) {
        variant.min_sdk = flavor.min_sdk;
        break;
    }
}
```

## Variant Naming

Variant names follow Gradle's conventions:

### No Flavors
```
debug
release
staging
```

### Single Dimension
```
freeDebug      // free + Debug
freeRelease    // free + Release
proDebug       // pro + Debug
proRelease     // pro + Release
```

### Multiple Dimensions
```
freeGoogleDebug      // free + Google + Debug
freeGoogleRelease    // free + Google + Release
freeAmazonDebug      // free + Amazon + Debug
freeAmazonRelease    // free + Amazon + Release
proGoogleDebug       // pro + Google + Debug
proGoogleRelease     // pro + Google + Release
proAmazonDebug       // pro + Amazon + Debug
proAmazonRelease     // pro + Amazon + Release
```

**Rules:**
- First flavor is lowercase
- Subsequent flavors are capitalized (first letter uppercase)
- Build type is capitalized
- No separators between parts

## Gradle Interoperability

Horcrux can parse existing Gradle files to extract variant configuration:

### From build.gradle

```gradle
android {
    compileSdk 34
    
    flavorDimensions "tier", "store"
    
    productFlavors {
        free {
            dimension "tier"
            applicationIdSuffix ".free"
        }
        pro {
            dimension "tier"
            applicationIdSuffix ".pro"
            versionCode 100
        }
        google {
            dimension "store"
        }
        amazon {
            dimension "store"
            applicationIdSuffix ".amazon"
        }
    }
    
    buildTypes {
        debug {
            debuggable true
            applicationIdSuffix ".debug"
        }
        release {
            minifyEnabled true
            shrinkResources true
        }
    }
}
```

### Parse with GradleParser

```cpp
#include "gradle_parser.h"

auto result = GradleParser::parse_build("build.gradle");
if (result) {
    auto& config = *result;
    
    std::cout << "Found " << config.build_variants.size() << " variants:\n";
    for (const auto& variant : config.build_variants) {
        std::cout << "  - " << variant.name 
                  << " (type: " << variant.build_type << ")\n";
        for (const auto& flavor : variant.flavors) {
            std::cout << "    Flavor: " << flavor << "\n";
        }
    }
}
```

## Deterministic Builds

Each variant produces a deterministic SHA-256 hash based on:

1. Variant name
2. Application ID
3. Version code and name
4. Build flags (debuggable, minify, shrink)
5. Source directories (sorted)
6. Resource directories (sorted)
7. Manifest files (sorted)

```cpp
// Compute hash for variant
std::string hash = variant.compute_hash();

// Same configuration always produces same hash
auto variant2 = /* ... same configuration ... */;
assert(variant.compute_hash() == variant2.compute_hash());
```

## Best Practices

### 1. Use Meaningful Dimension Names

```cpp
// Good
config.flavor_dimensions.push_back("tier");
config.flavor_dimensions.push_back("store");

// Avoid
config.flavor_dimensions.push_back("dim1");
config.flavor_dimensions.push_back("dim2");
```

### 2. Organize Flavors by Dimension

```cpp
// Define all flavors for first dimension
ProductFlavor free, pro;
free.dimension = "tier";
pro.dimension = "tier";

// Then all flavors for second dimension
ProductFlavor google, amazon;
google.dimension = "store";
amazon.dimension = "store";
```

### 3. Use Application ID Suffixes, Not Overrides

```cpp
// Preferred
free.application_id_suffix = ".free";

// Avoid (breaks base app ID)
free.application_id = "com.different.app";
```

### 4. Validate Configuration Before Building

```cpp
auto validation = config.validate();
if (!validation) {
    std::cerr << "Invalid configuration: " << validation.error() << "\n";
    return;
}
```

### 5. Cache Variant Hashes

```cpp
std::unordered_map<std::string, std::string> variant_hashes;

for (const auto& variant : matrix.variants) {
    variant_hashes[variant.name] = variant.compute_hash();
}

// Later, check if rebuild needed
if (cached_hash != variant.compute_hash()) {
    // Rebuild variant
}
```

## Performance Considerations

### Variant Matrix Size

The number of variants grows multiplicatively:

```
Total Variants = (flavors in dim1) × (flavors in dim2) × ... × (build types)
```

Examples:
- 0 dimensions, 2 build types = 2 variants
- 1 dimension (2 flavors), 2 build types = 4 variants
- 2 dimensions (2×2 flavors), 2 build types = 8 variants
- 3 dimensions (2×2×2 flavors), 2 build types = 16 variants

**Recommendation**: Limit to 2-3 dimensions for manageable matrix sizes.

### Incremental Builds

Each variant is cached independently:

```cpp
// Only rebuild variants with changed inputs
for (const auto& variant : matrix.variants) {
    auto current_hash = variant.compute_hash();
    if (cache.lookup(current_hash)) {
        std::cout << "Using cached build for " << variant.name << "\n";
        continue;
    }
    
    // Build variant
    build_variant(variant);
    cache.store(current_hash, build_output);
}
```

### Parallel Builds

Variants can be built in parallel:

```cpp
#include <future>
#include <vector>

std::vector<std::future<void>> futures;

for (const auto& variant : matrix.variants) {
    futures.push_back(std::async(std::launch::async, [&variant]() {
        build_variant(variant);
    }));
}

// Wait for all variants
for (auto& future : futures) {
    future.wait();
}
```

## Testing

Comprehensive test coverage ensures variant system correctness:

### Run Variant Tests

```bash
cd build
./bin/android_variant_test
```

**Test Coverage:**
- Build type properties
- Product flavor properties
- Variant name generation (single/multi-dimensional)
- Application ID merging
- Version merging
- Source/resource/manifest merging
- SDK version merging
- Build flags
- Deterministic hashing
- Configuration validation

### Run Gradle Parser Tests

```bash
cd build
./bin/gradle_parser_test
```

**Test Coverage:**
- Parse build.gradle with flavors
- Extract build types
- Extract flavor dimensions
- Generate variant matrix
- Multi-dimensional flavor support

## Limitations

Current limitations of the variant system:

1. **Build Script Plugins**: Complex Gradle plugins not automatically translated
2. **Dynamic Configuration**: Runtime-computed properties cannot be extracted
3. **Custom Variant Filtering**: Advanced variant filters not yet supported
4. **Split APKs**: ABI/density splits not yet supported

For unsupported features, manually configure variants using the API.

## See Also

- [Gradle Interoperability](gradle-interop.md)
- [Android Resources](android-resources.md)
- [Android Manifest Merger](android-manifest-merger.md)
- [Android Toolchain](android-toolchain.md)
- [Coding Standards](coding-standards.md)
