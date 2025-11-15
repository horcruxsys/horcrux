# Jetpack Compose Build Support

This document describes Horcrux's support for building Android applications with Jetpack Compose, ensuring hermetic, reproducible, and incremental builds for Compose-based projects.

## Overview

Horcrux provides native support for the Jetpack Compose compiler plugin, enabling:

- **Compose compiler plugin integration**: Full support for Compose compiler configuration
- **Deterministic Compose IR**: Content-addressable hashing for Compose transformations
- **Incremental Compose builds**: Only recompile changed composables
- **Compose metrics and reports**: Detailed compilation metrics and composable analysis
- **Stability configuration**: Cross-module stability annotations
- **Hermetic builds**: Explicit dependency tracking for Compose artifacts

## Compose Compiler Configuration

### Basic Setup

```cpp
#include "android_compose_compiler.h"
#include "android_kotlin_compiler.h"

using namespace horcrux::core;

// Configure Compose compiler
ComposeCompilerConfig compose_config;
compose_config.enabled = true;
compose_config.version = "1.5.4";
compose_config.kotlin_version = "1.9.20";
compose_config.plugin_jar = "/path/to/compose-compiler-1.5.4.jar";

// Configure Kotlin compiler with Compose
KotlinCompileConfig config;
config.kotlin_sources = /* your sources */;
config.language_version = "1.9";
config.jvm_target = "17";
config.api_version = "1.9";
config.compose_config = compose_config;

// Compile with Compose support
AndroidKotlinCompiler compiler(toolchain);
auto result = compiler.compile(config);
```

### ComposeCompilerConfig Options

| Field | Type | Description |
|-------|------|-------------|
| `enabled` | `bool` | Enable Compose compiler plugin (default: false) |
| `version` | `string` | Compose compiler version (e.g., "1.5.4") |
| `plugin_jar` | `path` | Path to Compose compiler plugin JAR |
| `kotlin_version` | `string` | Kotlin version used (e.g., "1.9.20") |
| `enable_metrics` | `bool` | Enable Compose compilation metrics |
| `metrics_output_dir` | `path` | Output directory for metrics files |
| `enable_reports` | `bool` | Enable Compose compiler reports |
| `reports_output_dir` | `path` | Output directory for reports |
| `enable_live_literals` | `bool` | Enable live literals (experimental) |
| `enable_source_information` | `bool` | Enable source info for debugging (default: true) |
| `enable_intrinsic_remember` | `bool` | Enable intrinsic remember optimization (default: true) |
| `suppress_kotlin_version_check` | `bool` | Suppress Kotlin version compatibility check |
| `stability_config_path` | `optional<path>` | Path to stability configuration file |
| `additional_options` | `vector<string>` | Additional Compose compiler options |

## Compose Metrics

Enable Compose metrics to analyze compilation performance and composable function optimization:

```cpp
ComposeCompilerConfig compose_config;
compose_config.enabled = true;
compose_config.version = "1.5.4";
compose_config.plugin_jar = "/path/to/compose-compiler.jar";

// Enable metrics
compose_config.enable_metrics = true;
compose_config.metrics_output_dir = "build/compose-metrics";

// Compile
auto result = compiler.compile(config);

// Parse metrics
auto metrics = compose_compiler::parse_compose_metrics(compose_config.metrics_output_dir);
if (metrics) {
    std::cout << "Total composables: " << metrics->total_composables << std::endl;
    std::cout << "Restartable: " << metrics->restartable_composables << std::endl;
    std::cout << "Skippable: " << metrics->skippable_composables << std::endl;
}
```

### Metrics Output

Compose metrics include:

- **Total composables**: Number of composable functions
- **Restartable composables**: Functions that can restart on state change
- **Skippable composables**: Functions that can skip recomposition
- **Readonly composables**: Functions that don't modify state
- **Lambda metrics**: Total and singleton lambdas
- **Group metrics**: Composition groups created
- **Compile time**: Compose-specific compilation time

## Compose Reports

Enable detailed reports for composable function analysis:

```cpp
compose_config.enable_reports = true;
compose_config.reports_output_dir = "build/compose-reports";
```

Reports provide detailed information about:

- Composable function signatures
- State parameters
- Stability inference results
- Optimization opportunities

## Stability Configuration

For multi-module projects, configure cross-module stability:

### Creating Stability Configuration

```cpp
std::vector<std::string> stable_types = {
    "com.example.myapp.data.User",
    "com.example.myapp.ui.Theme",
    "kotlinx.collections.immutable.ImmutableList"
};

auto result = compose_compiler::generate_stability_config(
    stable_types,
    "stability.conf"
);
```

### Using Stability Configuration

```cpp
ComposeCompilerConfig compose_config;
compose_config.enabled = true;
compose_config.stability_config_path = "stability.conf";
```

### Stability Configuration Format

```
# Stability configuration file
# One stable type per line

# Custom data classes
com.example.myapp.data.User
com.example.myapp.data.Product

# UI models
com.example.myapp.ui.UiState

# Third-party immutable types
kotlinx.collections.immutable.ImmutableList
kotlinx.collections.immutable.ImmutableSet
```

## Incremental Compose Compilation

Horcrux provides incremental Compose compilation through IR hashing:

```cpp
// Compute Compose IR hash
std::vector<std::filesystem::path> sources = {
    "src/MainActivity.kt",
    "src/ui/Theme.kt"
};

std::string ir_hash = compose_compiler::compute_compose_ir_hash(sources, compose_config);

// Check if recompilation needed
bool needs_recompile = (ir_hash != cached_hash);
```

The IR hash includes:

- Source file paths (deterministically sorted)
- Compose compiler version
- Kotlin version
- Compose configuration options
- Stability configuration

## Kotlin Version Compatibility

Horcrux validates Compose-Kotlin compatibility:

```cpp
bool compatible = compose_compiler::is_compose_kotlin_compatible(
    "1.5.4",  // Compose version
    "1.9.20"  // Kotlin version
);
```

### Compatibility Matrix

| Compose Version | Compatible Kotlin Versions |
|----------------|---------------------------|
| 1.4.x | 1.8.x, 1.9.x |
| 1.5.x | 1.9.x |
| 1.6.x | 1.9.x, 2.0.x |

## Generated Compose Classes

Scan and track generated Compose classes:

```cpp
auto generated_classes = compose_compiler::scan_compose_generated_classes(
    config.output_dir
);

for (const auto& cls : generated_classes) {
    std::cout << "Generated: " << cls << std::endl;
}
```

Generated classes include:

- `ComposableSingletons$*`: Singleton composable lambdas
- `*ComposerImpl`: Composer implementations
- `*Kt$Compose*`: Compose-transformed functions

## Plugin Options

Compose compiler plugin options are automatically generated:

```cpp
auto options = compose_compiler::build_compose_plugin_options(compose_config);

// Options include:
// -Xplugin=/path/to/compose-compiler.jar
// -P plugin:androidx.compose.compiler.plugins.kotlin:metricsDestination=...
// -P plugin:androidx.compose.compiler.plugins.kotlin:reportsDestination=...
// -P plugin:androidx.compose.compiler.plugins.kotlin:liveLiterals=false
// -P plugin:androidx.compose.compiler.plugins.kotlin:sourceInformation=true
// -P plugin:androidx.compose.compiler.plugins.kotlin:intrinsicRemember=true
```

## Complete Example

```cpp
#include "android_compose_compiler.h"
#include "android_kotlin_compiler.h"
#include "android_toolchain.h"

using namespace horcrux::core;

// Detect Android toolchain
auto toolchain_result = AndroidToolchainDetector::detect();
if (!toolchain_result) {
    std::cerr << "Android toolchain not found" << std::endl;
    return 1;
}

// Configure Compose compiler
ComposeCompilerConfig compose_config;
compose_config.enabled = true;
compose_config.version = "1.5.4";
compose_config.kotlin_version = "1.9.20";
compose_config.plugin_jar = "~/.gradle/caches/modules-2/files-2.1/"
                            "androidx.compose.compiler/compiler/1.5.4/"
                            "compose-compiler-1.5.4.jar";

// Enable metrics and reports
compose_config.enable_metrics = true;
compose_config.metrics_output_dir = "build/compose-metrics";
compose_config.enable_reports = true;
compose_config.reports_output_dir = "build/compose-reports";

// Enable optimizations
compose_config.enable_source_information = true;
compose_config.enable_intrinsic_remember = true;

// Configure Kotlin compilation
KotlinCompileConfig config;

// Add Compose sources
KotlinSourceFile main_activity;
main_activity.path = "src/MainActivity.kt";
main_activity.package_name = "com.example.myapp";
main_activity.content_hash = kotlin_incremental::compute_source_hash(main_activity.path);
config.kotlin_sources.push_back(main_activity);

KotlinSourceFile theme;
theme.path = "src/ui/Theme.kt";
theme.package_name = "com.example.myapp.ui";
theme.content_hash = kotlin_incremental::compute_source_hash(theme.path);
config.kotlin_sources.push_back(theme);

// Configure Kotlin options
config.language_version = "1.9";
config.api_version = "1.9";
config.jvm_target = "17";
config.output_dir = "build/classes/kotlin/main";

// Add Compose dependencies
config.classpath.push_back("libs/compose-runtime.jar");
config.classpath.push_back("libs/compose-ui.jar");
config.classpath.push_back("libs/kotlin-stdlib.jar");

// Set Android bootclasspath
config.bootclasspath = toolchain_result->platforms[0].android_jar_path;

// Add Compose configuration
config.compose_config = compose_config;

// Validate configuration
auto validation = AndroidKotlinCompiler::validate_config(config);
if (!validation) {
    std::cerr << "Invalid configuration" << std::endl;
    return 1;
}

// Create compiler and compile
AndroidKotlinCompiler compiler(*toolchain_result);
auto result = compiler.compile(config);

if (!result) {
    std::cerr << "Compilation failed: " << to_string(result.error()) << std::endl;
    return 1;
}

std::cout << "Compilation successful!" << std::endl;
std::cout << "Compiled " << result->class_files.size() << " classes" << std::endl;

// Parse and display metrics
auto metrics = compose_compiler::parse_compose_metrics(compose_config.metrics_output_dir);
if (metrics) {
    std::cout << "\nCompose Metrics:" << std::endl;
    std::cout << "  Total composables: " << metrics->total_composables << std::endl;
    std::cout << "  Restartable: " << metrics->restartable_composables << std::endl;
    std::cout << "  Skippable: " << metrics->skippable_composables << std::endl;
    std::cout << "  Readonly: " << metrics->readonly_composables << std::endl;
}
```

## Best Practices

### 1. Always Use Stability Configuration for Multi-Module Projects

```cpp
// Define stable types in stability.conf
compose_config.stability_config_path = "stability.conf";
```

### 2. Enable Metrics in Development

```cpp
#ifdef DEBUG
compose_config.enable_metrics = true;
compose_config.enable_reports = true;
#endif
```

### 3. Use Intrinsic Remember

```cpp
// Always enable for better performance
compose_config.enable_intrinsic_remember = true;
```

### 4. Version Compatibility

```cpp
// Validate compatibility before compilation
if (!compose_compiler::is_compose_kotlin_compatible(
        compose_config.version, 
        config.language_version)) {
    std::cerr << "Incompatible Compose/Kotlin versions" << std::endl;
    return 1;
}
```

### 5. Incremental Compilation

```cpp
// Always compute and cache IR hash
std::string ir_hash = compose_compiler::compute_compose_ir_hash(sources, compose_config);
cache.store("compose_ir_hash", ir_hash);
```

## Performance Characteristics

- **First build**: Full Compose transformation applied
- **Incremental build**: Only changed composables recompiled
- **Cache hit**: Zero Compose compilation time
- **IR hashing**: O(n) where n = number of source files
- **Metrics parsing**: O(m) where m = number of composables

## Testing

Horcrux includes comprehensive tests for Compose support:

```bash
# Run Compose compiler tests
cd build
ctest -R ComposeCompiler

# Run Compose-Kotlin integration tests
ctest -R ComposeKotlinIntegration
```

## Future Enhancements

- [ ] Kotlin K2 compiler support for Compose
- [ ] Strong skipping mode configuration
- [ ] Compose compiler daemon integration
- [ ] Multi-platform Compose support
- [ ] Enhanced stability inference

## See Also

- [Android Kotlin Compiler Rules](android-compiler-rules.md)
- [Android Toolchain Detection](android-toolchain.md)
- [Architecture](architecture.md)
- [Coding Standards](coding-standards.md)

## References

- [Jetpack Compose Compiler](https://developer.android.com/jetpack/compose/compiler)
- [Compose Compiler Metrics](https://github.com/androidx/androidx/blob/androidx-main/compose/compiler/design/compiler-metrics.md)
- [Compose Stability](https://developer.android.com/jetpack/compose/performance/stability)
