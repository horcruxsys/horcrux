# Android Build Sandbox & Hermeticity

This document describes Horcrux's Android build sandboxing system that ensures hermetic and reproducible builds for Android applications.

## Overview

Horcrux implements a comprehensive sandboxing system for Android build tools to ensure:

- **Hermetic builds**: Builds produce identical artifacts on any machine
- **Filesystem isolation**: No external filesystem access beyond declared inputs
- **Process isolation**: Build tools run in isolated environments
- **Violation detection**: Automatic detection of sandbox violations
- **Reproducibility**: Same inputs always produce identical outputs

## Architecture

### Components

1. **AndroidSandbox** - Core sandbox execution engine
2. **SandboxConfig** - Configuration for sandbox execution
3. **MountRule** - Filesystem mount rules (read-only/read-write)
4. **SandboxResult** - Execution result with violation tracking
5. **SandboxGuard** - RAII helper for cleanup

### Isolation Features

The Android sandbox provides multiple levels of isolation:

#### Filesystem Isolation
- **Read-only SDK mounts**: Android SDK is mounted read-only
- **Read-only source mounts**: Source files are mounted read-only
- **Writable scratch directories**: Build output directories are writable
- **TmpFS for temporary files**: In-memory temporary filesystem

#### Process Isolation (Linux)
When running on Linux with appropriate privileges, the sandbox uses Linux namespaces:
- **Mount namespace** (`CLONE_NEWNS`): Isolate filesystem mounts
- **PID namespace** (`CLONE_NEWPID`): Isolate process tree
- **Network namespace** (`CLONE_NEWNET`): Disable network access
- **IPC namespace** (`CLONE_NEWIPC`): Isolate inter-process communication
- **UTS namespace** (`CLONE_NEWUTS`): Isolate hostname

#### Fallback Mode
For environments without namespace support (non-Linux or unprivileged), the sandbox falls back to basic execution with violation detection.

## Usage

### Enabling Sandbox for Kotlin Compilation

```cpp
#include "android_kotlin_compiler.h"
#include "android_toolchain.h"

using namespace horcrux::core;

// Detect Android toolchain
auto toolchain = AndroidToolchainDetector::detect();
if (!toolchain) {
    // Handle error
}

// Create Kotlin compiler
AndroidKotlinCompiler compiler(*toolchain);

// Configure compilation
KotlinCompileConfig config;
config.kotlin_sources = {/* your sources */};
config.output_dir = "build/kotlin/classes";

// Enable sandbox
config.enable_sandbox = true;
config.sdk_path = "/path/to/android/sdk";

// Compile with sandbox
auto result = compiler.compile(config);
if (!result) {
    std::cerr << "Compilation failed: " << to_string(result.error()) << std::endl;
}
```

### Enabling Sandbox for Java Compilation

```cpp
#include "android_java_compiler.h"

JavaCompileConfig config;
config.sources = {/* your sources */};
config.output_dir = "build/java/classes";

// Enable sandbox
config.enable_sandbox = true;
config.sdk_path = "/path/to/android/sdk";

AndroidJavaCompiler compiler(toolchain);
auto result = compiler.compile(config);
```

### Direct Sandbox Usage

For custom build tools, you can use the `AndroidSandbox` directly:

```cpp
#include "android_sandbox.h"

// Create sandbox configuration
SandboxConfig config = AndroidSandbox::create_android_build_sandbox(
    "/usr/bin/tool",                  // Executable
    {"arg1", "arg2"},                 // Arguments
    "/path/to/sdk",                   // SDK path (read-only)
    "/path/to/source",                // Source path (read-only)
    "/path/to/output"                 // Output path (read-write)
);

// Execute in sandbox
AndroidSandbox sandbox;
auto result = sandbox.execute(config);

if (!result) {
    std::cerr << "Sandbox execution failed: " << to_string(result.error()) << std::endl;
} else {
    std::cout << "Exit code: " << result->exit_code << std::endl;
    std::cout << "Stdout: " << result->stdout_output << std::endl;
    
    // Check for violations
    if (!result->violations.empty()) {
        std::cerr << "Detected " << result->violations.size() << " violations:" << std::endl;
        for (const auto& violation : result->violations) {
            std::cerr << "  - " << violation.description << std::endl;
        }
    }
}
```

### Mount Rules

Create custom mount rules for specific needs:

```cpp
SandboxConfig config;

// Read-only SDK
config.mounts.push_back(
    MountRule::read_only("/android/sdk", "/sandbox/sdk"));

// Read-only source
config.mounts.push_back(
    MountRule::read_only("/project/src", "/sandbox/src"));

// Writable output
config.mounts.push_back(
    MountRule::read_write("/project/build", "/sandbox/build"));

// TmpFS for temporary files
config.mounts.push_back(
    MountRule::tmpfs("/sandbox/tmp"));

// Optional mount (doesn't fail if source doesn't exist)
MountRule optional_mount = MountRule::read_only("/opt/tools", "/sandbox/tools");
optional_mount.optional = true;
config.mounts.push_back(optional_mount);
```

## Violation Detection

The sandbox automatically detects common violations:

### Types of Violations

1. **UnauthorizedFileAccess**: Accessed file outside allowed paths
2. **UnauthorizedNetworkAccess**: Attempted network access
3. **UnauthorizedProcessSpawn**: Spawned unauthorized subprocess
4. **UnauthorizedWrite**: Write to read-only location

### Violation Patterns

The violation detector scans build tool output for patterns like:

- `Permission denied`
- `Access denied`
- `Read-only file system`
- `Network unreachable`
- `Connection refused`

### Handling Violations

```cpp
SandboxConfig config;
config.detect_violations = true;
config.fail_on_violation = true;  // Fail build on violation

auto result = sandbox.execute(config);

if (!result) {
    if (result.error() == SandboxError::ViolationDetected) {
        std::cerr << "Build failed due to sandbox violations" << std::endl;
    }
}
```

## Hermetic Builds

Hermetic builds ensure reproducibility across different machines and environments.

### Requirements for Hermetic Builds

1. **Explicit dependencies**: All inputs must be declared
2. **Read-only inputs**: SDK and source files are read-only
3. **Isolated execution**: No access to external filesystem
4. **Deterministic ordering**: Files processed in sorted order
5. **Content-addressable caching**: Outputs identified by hash

### Verification

To verify hermetic builds, compile the same code on different machines:

```bash
# Machine 1
horcrux-cli build --sandbox //app:compile
sha256sum build/output.jar

# Machine 2
horcrux-cli build --sandbox //app:compile
sha256sum build/output.jar

# Hashes should be identical
```

## Sandboxed Android Build Tools

### Kotlin Compiler (kotlinc)

**Status**: ✅ Integrated

**Mount Rules**:
- SDK: Read-only (`/android/sdk`)
- Sources: Read-only (`/project/src`)
- Output: Read-write (`/project/build/kotlin`)
- Classpath JARs: Read-only

**Example**:
```cpp
KotlinCompileConfig config;
config.enable_sandbox = true;
config.sdk_path = "/android/sdk";
auto result = compiler.compile(config);
```

### Java Compiler (javac)

**Status**: ✅ Config Support Added

**Mount Rules**:
- SDK: Read-only (`/android/sdk`)
- Sources: Read-only (`/project/src`)
- Output: Read-write (`/project/build/java`)
- Classpath JARs: Read-only

### Resource Compiler (aapt2)

**Status**: 🔄 In Progress

**Mount Rules**:
- SDK: Read-only (for framework resources)
- Resources: Read-only (`/project/res`)
- Output: Read-write (`/project/build/resources`)

### DEX Compiler (d8/r8)

**Status**: 🔄 In Progress

**Mount Rules**:
- SDK: Read-only (for platform JARs)
- Input JARs: Read-only
- Output: Read-write (`/project/build/dex`)

### APK Packager (zipalign)

**Status**: 🔄 In Progress

**Mount Rules**:
- Input APK: Read-only
- Output APK: Read-write

### APK Signer (apksigner)

**Status**: 🔄 In Progress

**Mount Rules**:
- Input APK: Read-only
- Keystore: Read-only
- Output APK: Read-write

## Platform Support

### Linux

Full sandbox support with Linux namespaces when running with appropriate privileges.

**Requirements**:
- Linux kernel with namespace support
- `CAP_SYS_ADMIN` capability or user namespaces enabled

**Features**:
- Full process isolation
- Mount namespace isolation
- Network isolation
- PID namespace

### macOS

Fallback mode with violation detection.

**Features**:
- Basic process execution
- Violation pattern detection
- Working directory isolation

### Windows

Fallback mode with violation detection.

**Features**:
- Basic process execution
- Violation pattern detection
- Working directory isolation

## Configuration Reference

### SandboxConfig

```cpp
struct SandboxConfig {
  // Command execution
  std::filesystem::path executable;
  std::vector<std::string> arguments;
  std::filesystem::path working_dir;

  // Mount rules
  std::vector<MountRule> mounts;

  // Environment variables
  std::vector<std::pair<std::string, std::string>> env_vars;

  // Isolation features (Linux only)
  bool enable_network_isolation = true;
  bool enable_pid_namespace = true;
  bool enable_mount_namespace = true;
  bool enable_ipc_namespace = true;
  bool enable_uts_namespace = true;

  // Violation detection
  bool detect_violations = true;
  bool fail_on_violation = true;

  // Resource limits
  std::optional<size_t> max_memory_bytes;
  std::optional<std::chrono::seconds> timeout;

  // Debugging
  bool verbose = false;
};
```

### MountRule

```cpp
struct MountRule {
  std::filesystem::path source;
  std::filesystem::path target;
  MountType type;  // ReadOnly, ReadWrite, TmpFs
  bool optional = false;
};
```

## Best Practices

### 1. Always Enable Sandbox for Release Builds

```cpp
config.enable_sandbox = true;
config.sdk_path = detect_android_sdk();
```

### 2. Use Read-Only Mounts for Inputs

```cpp
config.mounts.push_back(
    MountRule::read_only(source_dir, "/sandbox/src"));
```

### 3. Enable Violation Detection

```cpp
config.detect_violations = true;
config.fail_on_violation = true;
```

### 4. Use TmpFS for Temporary Files

```cpp
config.mounts.push_back(MountRule::tmpfs("/tmp"));
```

### 5. Set Resource Limits

```cpp
config.max_memory_bytes = 4 * 1024 * 1024 * 1024;  // 4GB
config.timeout = std::chrono::minutes(10);
```

### 6. Test Hermeticity

Regularly verify builds produce identical artifacts:

```bash
# Build twice and compare
horcrux-cli build --sandbox //app:all
cp -r build build1
horcrux-cli clean
horcrux-cli build --sandbox //app:all
diff -r build build1
```

## Troubleshooting

### Permission Denied Errors

If you see "Operation not permitted" when creating namespaces:

1. Run with elevated privileges (sudo) if appropriate
2. Enable user namespaces: `sysctl -w kernel.unprivileged_userns_clone=1`
3. Or disable namespace isolation: `config.enable_mount_namespace = false;`

### Sandbox Violations

If violation detection reports false positives:

1. Review the violation description
2. Add optional mounts for legitimate dependencies
3. Or disable specific violation detection if needed

### Performance Impact

Sandboxing adds minimal overhead (typically < 5%):

1. First run: Slightly slower due to mount setup
2. Subsequent runs: Nearly identical to non-sandboxed
3. Most overhead is from process creation, not sandboxing

## Future Enhancements

- [ ] Container-based sandboxing (Docker/Podman)
- [ ] Remote sandbox execution (distributed builds)
- [ ] Sandbox cache warmup for faster first builds
- [ ] Windows Sandbox integration
- [ ] macOS Sandbox profiles
- [ ] Resource usage tracking and limits
- [ ] Audit logging for compliance

## See Also

- [Android Toolchain Detection](android-toolchain.md)
- [Android Compiler Rules](android-compiler-rules.md)
- [Coding Standards](coding-standards.md)
- [Architecture](architecture.md)
