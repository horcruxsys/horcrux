# Android Java/Kotlin Compiler Rules

This document describes Horcrux's Java and Kotlin compiler rules for Android application development, providing hermetic and reproducible builds for mixed-language Android projects.

## Overview

Horcrux provides native support for compiling Java and Kotlin sources for Android applications. The compiler rules ensure:

- **Hermetic builds**: All dependencies are explicitly declared and tracked
- **Incremental compilation**: Only changed sources are recompiled using Merkle signatures
- **Deterministic builds**: Same inputs always produce identical outputs
- **Mixed-language support**: Seamless compilation of Kotlin and Java together
- **Annotation processing**: Support for KAPT and KSP
- **Content-addressable caching**: Build artifacts are cached by their SHA-256 hash

## Java Compiler Rule

The `AndroidJavaCompiler` provides hermetic Java compilation for Android projects.

### Features

- ✅ Java source compilation with configurable source/target versions
- ✅ Android bootclasspath support (android.jar)
- ✅ Classpath management with platform-specific separators
- ✅ Incremental compilation based on content hashes
- ✅ Annotation processor support
- ✅ Deterministic source file ordering
- ✅ Comprehensive error handling

### Usage Example

```cpp
#include "android_java_compiler.h"
#include "android_toolchain.h"

using namespace horcrux::core;

// Detect Android toolchain
auto toolchain_result = AndroidToolchainDetector::detect();
if (!toolchain_result) {
    // Handle error
}

// Create compiler
AndroidJavaCompiler compiler(*toolchain_result);

// Configure compilation
JavaCompileConfig config;

// Add source files
JavaSourceFile main_source;
main_source.path = "src/com/example/MainActivity.java";
main_source.package_name = "com.example";
main_source.content_hash = java_incremental::compute_source_hash(main_source.path);
config.sources.push_back(std::move(main_source));

// Configure versions
config.source_version = "17";
config.target_version = "17";

// Set output directory
config.output_dir = "build/classes/java/main";

// Add classpath
config.classpath.push_back("libs/androidx-core.jar");
config.classpath.push_back("libs/material.jar");

// Set Android bootclasspath
auto android_jar = toolchain_result->platforms[0].android_jar_path;
config.bootclasspath = android_jar;

// Enable incremental compilation
config.incremental = true;

// Compile
auto result = compiler.compile(config);
if (!result) {
    std::cerr << "Compilation failed: " << to_string(result.error()) << std::endl;
    return;
}

std::cout << "Compiled " << result->class_files.size() << " classes in "
          << result->compilation_time.count() << "ms" << std::endl;
std::cout << "Compilation hash: " << result->compilation_hash << std::endl;
```

### Configuration Options

#### JavaCompileConfig

| Field | Type | Description |
|-------|------|-------------|
| `sources` | `vector<JavaSourceFile>` | Java source files to compile |
| `classpath` | `vector<filesystem::path>` | Classpath entries (JARs, directories) |
| `output_dir` | `filesystem::path` | Output directory for .class files |
| `source_version` | `string` | Java source version (e.g., "11", "17") |
| `target_version` | `string` | Java target version (e.g., "11", "17") |
| `processor_path` | `vector<filesystem::path>` | Annotation processor classpath |
| `processor_options` | `vector<string>` | Annotation processor options |
| `javac_options` | `vector<string>` | Additional javac options |
| `incremental` | `bool` | Enable incremental compilation (default: true) |
| `verbose` | `bool` | Enable verbose output (default: false) |
| `bootclasspath` | `optional<filesystem::path>` | Bootclasspath (android.jar for Android) |

### Incremental Compilation

The Java compiler uses SHA-256 content hashing for incremental compilation:

1. **Source hashing**: Each source file's content is hashed
2. **Configuration hashing**: Classpath, versions, and options are hashed
3. **Merkle signature**: Combined hash identifies unique compilation state
4. **Cache lookup**: If hash matches cached state, compilation is skipped
5. **State persistence**: Compilation state saved to `.horcrux_java_state`

```cpp
// Compute compilation hash (Merkle signature)
std::string hash = AndroidJavaCompiler::compute_compilation_hash(config);

// Check if compilation is needed
bool needs_compile = compiler.is_compilation_needed(config, cached_hash);
```

### Source File Parsing

The compiler can parse Java sources to extract metadata:

```cpp
auto source_result = AndroidJavaCompiler::parse_java_source("MainActivity.java");
if (source_result) {
    std::cout << "Package: " << source_result->package_name << std::endl;
    std::cout << "Imports: " << source_result->imports.size() << std::endl;
    std::cout << "Hash: " << source_result->content_hash << std::endl;
}
```

## Kotlin Compiler Rule

The `AndroidKotlinCompiler` provides hermetic Kotlin compilation for Android projects, including mixed Kotlin/Java compilation.

### Features

- ✅ Kotlin source compilation with language/API version control
- ✅ Mixed Kotlin/Java compilation in a single invocation
- ✅ KAPT (Kotlin Annotation Processing Tool) support
- ✅ KSP (Kotlin Symbol Processing) support
- ✅ Incremental compilation based on content hashes
- ✅ JVM target configuration
- ✅ Deterministic source file ordering
- ✅ Comprehensive error handling

### Usage Example

```cpp
#include "android_kotlin_compiler.h"
#include "android_toolchain.h"

using namespace horcrux::core;

// Detect Android toolchain
auto toolchain_result = AndroidToolchainDetector::detect();
if (!toolchain_result) {
    // Handle error
}

// Create compiler
AndroidKotlinCompiler compiler(*toolchain_result);

// Configure compilation
KotlinCompileConfig config;

// Add Kotlin source files
KotlinSourceFile main_source;
main_source.path = "src/com/example/MainActivity.kt";
main_source.package_name = "com.example";
main_source.content_hash = kotlin_incremental::compute_source_hash(main_source.path);
config.kotlin_sources.push_back(std::move(main_source));

// Add Java source files (mixed compilation)
config.java_sources.push_back("src/com/example/Utils.java");

// Configure versions
config.language_version = "1.9";
config.api_version = "1.9";
config.jvm_target = "17";

// Set output directory
config.output_dir = "build/classes/kotlin/main";

// Add classpath
config.classpath.push_back("libs/kotlin-stdlib.jar");
config.classpath.push_back("libs/androidx-core.jar");

// Set Android bootclasspath
auto android_jar = toolchain_result->platforms[0].android_jar_path;
config.bootclasspath = android_jar;

// Enable incremental compilation
config.incremental = true;

// Compile
auto result = compiler.compile(config);
if (!result) {
    std::cerr << "Compilation failed: " << to_string(result.error()) << std::endl;
    return;
}

std::cout << "Compiled " << result->class_files.size() << " classes in "
          << result->compilation_time.count() << "ms" << std::endl;
std::cout << "Generated " << result->generated_sources.size() << " source files" << std::endl;
```

### Configuration Options

#### KotlinCompileConfig

| Field | Type | Description |
|-------|------|-------------|
| `kotlin_sources` | `vector<KotlinSourceFile>` | Kotlin source files to compile |
| `java_sources` | `vector<filesystem::path>` | Java source files (mixed compilation) |
| `classpath` | `vector<filesystem::path>` | Classpath entries (JARs, directories) |
| `output_dir` | `filesystem::path` | Output directory for .class files |
| `language_version` | `string` | Kotlin language version (e.g., "1.9") |
| `jvm_target` | `string` | JVM target version (e.g., "17") |
| `api_version` | `string` | Kotlin API version (e.g., "1.9") |
| `plugin_classpath` | `vector<filesystem::path>` | Plugin classpath (KAPT, KSP) |
| `plugin_options` | `vector<string>` | Plugin options |
| `kotlinc_options` | `vector<string>` | Additional kotlinc options |
| `incremental` | `bool` | Enable incremental compilation (default: true) |
| `verbose` | `bool` | Enable verbose output (default: false) |
| `bootclasspath` | `optional<filesystem::path>` | Bootclasspath (android.jar for Android) |
| `kapt_config` | `optional<KaptConfig>` | KAPT configuration |
| `ksp_config` | `optional<KspConfig>` | KSP configuration |

### KAPT Support

Kotlin Annotation Processing Tool (KAPT) is supported for annotation processors:

```cpp
KotlinCompileConfig config;
// ... basic configuration ...

// Configure KAPT
KotlinCompileConfig::KaptConfig kapt;
kapt.enabled = true;
kapt.processor_classpath.push_back("libs/dagger-compiler.jar");
kapt.generated_sources_dir = "build/generated/source/kapt/main";
kapt.generated_stubs_dir = "build/tmp/kapt/stubs/main";
kapt.processor_options.push_back("dagger.fastInit=enabled");

config.kapt_config = std::move(kapt);
```

### KSP Support

Kotlin Symbol Processing (KSP) is supported as a more efficient alternative to KAPT:

```cpp
KotlinCompileConfig config;
// ... basic configuration ...

// Configure KSP
KotlinCompileConfig::KspConfig ksp;
ksp.enabled = true;
ksp.processor_classpath.push_back("libs/moshi-kotlin-codegen.jar");
ksp.output_dir = "build/generated/ksp/main";
ksp.processor_options.push_back("option.generateProguard=true");

config.ksp_config = std::move(ksp);
```

### Mixed Kotlin/Java Compilation

The Kotlin compiler can handle both Kotlin and Java sources in a single compilation:

```cpp
// Separate mixed sources
auto [kotlin_files, java_files] = kotlin_java_interop::separate_sources(all_sources);

// Add to config
for (const auto& kt_file : kotlin_files) {
    auto source = AndroidKotlinCompiler::parse_kotlin_source(kt_file);
    if (source) {
        config.kotlin_sources.push_back(*source);
    }
}

config.java_sources = java_files;

// Compile all together
auto result = compiler.compile(config);
```

## Classpath Management

Both compilers provide helper functions for classpath management:

### Building Classpath Strings

```cpp
std::vector<std::filesystem::path> classpath = {
    "libs/kotlin-stdlib.jar",
    "libs/androidx-core.jar",
    "build/intermediates/classes"
};

// Build platform-specific classpath string
std::string cp_string = java_classpath::build_classpath_string(classpath);
// Linux/Mac: "libs/kotlin-stdlib.jar:libs/androidx-core.jar:build/intermediates/classes"
// Windows:   "libs/kotlin-stdlib.jar;libs/androidx-core.jar;build/intermediates/classes"
```

### Parsing Classpath Strings

```cpp
std::string cp_string = "lib1.jar:lib2.jar:lib3.jar";
auto classpath = java_classpath::parse_classpath_string(cp_string);
// Returns: vector of filesystem::path
```

### Validating Classpath

```cpp
if (!java_classpath::validate_classpath(classpath)) {
    std::cerr << "One or more classpath entries do not exist" << std::endl;
}
```

## Error Handling

Both compilers use `tl::expected` for error handling:

### Java Compiler Errors

```cpp
enum class JavaCompilerError {
  CompilerNotFound,        // javac not found
  InvalidSourceFile,       // Source file missing or unreadable
  InvalidClasspath,        // Classpath entry doesn't exist
  CompilationFailed,       // Compilation errors
  InvalidConfiguration,    // Invalid config
  IoError,                 // I/O error
  UnknownError            // Unknown error
};
```

### Kotlin Compiler Errors

```cpp
enum class KotlinCompilerError {
  CompilerNotFound,        // kotlinc not found
  InvalidSourceFile,       // Source file missing or unreadable
  InvalidClasspath,        // Classpath entry doesn't exist
  CompilationFailed,       // Compilation errors
  InvalidConfiguration,    // Invalid config
  IoError,                 // I/O error
  UnknownError            // Unknown error
};
```

### Error Handling Pattern

```cpp
auto result = compiler.compile(config);
if (!result) {
    // Handle error
    std::cerr << "Error: " << to_string(result.error()) << std::endl;
    
    switch (result.error()) {
        case JavaCompilerError::CompilerNotFound:
            std::cerr << "Please install JDK and set JAVA_HOME" << std::endl;
            break;
        case JavaCompilerError::CompilationFailed:
            std::cerr << "Check compiler output for details" << std::endl;
            break;
        // ... handle other errors
    }
    
    return;
}

// Use successful result
auto& compile_result = *result;
```

## Content-Addressable Storage

Both compilers integrate with Horcrux's CAS (Content-Addressable Storage) system:

1. **Hash computation**: Each compilation produces a unique SHA-256 hash
2. **Cache lookup**: Before compilation, check if artifacts exist in CAS
3. **Cache storage**: After compilation, store artifacts in CAS
4. **Artifact retrieval**: Retrieve cached artifacts using hash

```cpp
// Get compilation hash
std::string hash = result->compilation_hash;

// Store in CAS
LocalCache cache("~/.horcrux/cache");
for (const auto& class_file : result->class_files) {
    auto content = read_file(class_file);
    cache.store(hash + "/" + class_file.filename(), content);
}

// Later, retrieve from CAS
if (cache.contains(hash)) {
    auto artifact = cache.lookup(hash);
    // Use cached artifact
}
```

## Deterministic Builds

Both compilers ensure deterministic builds:

### Source File Ordering

All source files are sorted before compilation to ensure consistent ordering:

```cpp
// Java compiler - deterministic ordering
std::vector<std::string> source_paths;
for (const auto& source : config.sources) {
    source_paths.push_back(source.path.string());
}
std::sort(source_paths.begin(), source_paths.end());

// Kotlin compiler - deterministic ordering
std::sort(kotlin_paths.begin(), kotlin_paths.end());
std::sort(java_paths.begin(), java_paths.end());
```

### Hash Computation

All configuration elements are included in hash computation in a deterministic order:

1. Source files (sorted)
2. Classpath entries (sorted)
3. Bootclasspath
4. Version settings
5. Compiler options (sorted)
6. Plugin configuration

## Integration with Android Toolchain

Both compilers integrate with the Android toolchain detection system:

```cpp
// Detect Android toolchain
auto toolchain = AndroidToolchainDetector::detect();
if (!toolchain) {
    std::cerr << "Android toolchain not found" << std::endl;
    return;
}

// Use detected android.jar
if (!toolchain->platforms.empty()) {
    config.bootclasspath = toolchain->platforms[0].android_jar_path;
}

// Use detected Java SDK
if (toolchain->java_sdk) {
    std::cout << "Using Java: " << toolchain->java_sdk->version << std::endl;
}
```

## Performance Considerations

### Incremental Compilation

- **First build**: All sources compiled
- **Subsequent builds**: Only changed sources recompiled
- **Cache hits**: Zero compilation time when nothing changed

### Parallel Compilation

For multi-module projects, compile modules in parallel:

```cpp
std::vector<std::future<KotlinCompileResult>> futures;

for (const auto& module_config : module_configs) {
    futures.push_back(std::async(std::launch::async, [&]() {
        AndroidKotlinCompiler compiler(toolchain);
        return compiler.compile(module_config);
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

### 2. Use Hermetic Dependencies

```cpp
// Good: Explicit, versioned dependencies
config.classpath.push_back("libs/kotlin-stdlib-1.9.0.jar");

// Bad: Implicit dependencies from PATH
// Don't rely on system-installed libraries
```

### 3. Validate Configuration

```cpp
auto validation = AndroidJavaCompiler::validate_config(config);
if (!validation) {
    std::cerr << "Invalid config: " << to_string(validation.error()) << std::endl;
    return;
}
```

### 4. Compute Hashes for All Sources

```cpp
for (auto& source : config.sources) {
    source.content_hash = java_incremental::compute_source_hash(source.path);
}
```

### 5. Cache Compilation Results

```cpp
// Save compilation state
if (config.incremental) {
    std::filesystem::path state_file = config.output_dir / ".horcrux_state";
    java_incremental::save_compilation_state(state_file, config, result->compilation_hash);
}
```

## Testing

Both compilers have comprehensive unit test coverage:

### Java Compiler Tests (22 tests)

- Error handling
- Classpath management
- Incremental compilation
- Source parsing
- Configuration validation
- Hash computation

### Kotlin Compiler Tests (25 tests)

- Error handling
- Kotlin/Java interop
- Incremental compilation
- Source parsing
- Configuration validation
- Hash computation
- KAPT/KSP configuration

Run tests:

```bash
cd build
ctest --output-on-failure -R "JavaCompiler|KotlinCompiler"
```

## Future Enhancements

- [ ] Kotlin K2 compiler support
- [ ] Remote compilation caching
- [ ] Distributed compilation
- [ ] Build cache warming
- [ ] Compiler daemon support
- [ ] Source set dependencies
- [ ] Multi-platform Kotlin support

## See Also

- [Android Toolchain Detection](android-toolchain.md)
- [Coding Standards](coding-standards.md)
- [Architecture](architecture.md)
- [Local Cache](../src/core/local_cache.h)
