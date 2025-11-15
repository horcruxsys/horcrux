# 🧱 Horcrux — The Next-Generation Universal Build System

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Build Status](https://github.com/horcruxsys/horcrux/actions/workflows/build.yml/badge.svg)](https://github.com/horcruxsys/horcrux/actions/workflows/build.yml)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)
[![Code of Conduct](https://img.shields.io/badge/code%20of%20conduct-contributor%20covenant-purple.svg)](CODE_OF_CONDUCT.md)

Horcrux is a **modern, high-performance, memory-safe, and developer-friendly build system**, written in **C++23**, designed to build **any project in any language** — from C++ to Rust, Python to Go, and everything in between.

It aims to be **10x faster**, **more deterministic**, and **easier to extend** than existing build systems like **Bazel**, **Buck**, or **CMake**, while maintaining correctness, reproducibility, and minimal resource consumption.

## 📋 Table of Contents

- [Vision](#-vision)
- [Why Horcrux?](#-why-horcrux)
- [Use Cases](#-use-cases)
- [Goals](#-goals)
- [Core Architecture](#-core-architecture)
- [Getting Started](#-getting-started)
- [Usage](#-usage)
- [Design Guidelines](#-design-guidelines)
- [Benchmarks](#-benchmark-philosophy)
- [Roadmap](#-roadmap)
- [Contributing](#-contributing)
- [Getting Help](#-getting-help)
- [License](#-license)

## 🚀 Vision

To create the world’s most **intelligent and efficient build system** that understands source code, adapts to developer workflows, and ensures **accurate, reproducible, and incremental builds** — automatically.

Horcrux is built on three foundational principles:

1. **Correctness First** — Builds must always produce consistent, verifiable artifacts.  
2. **Speed by Design** — Every computation, cache, and dependency is optimized for minimal latency.  
3. **Developer Empathy** — Configuration should be intuitive, debuggable, and transparent.

## 🎯 Why Horcrux?

Build systems are the backbone of software development, yet many are slow, complex, or unreliable. Horcrux addresses these pain points:

- **Slow builds** waste developer time and break flow states
- **Non-deterministic builds** make debugging nightmares and deployment risky
- **Complex configurations** create steep learning curves and maintenance burdens
- **Limited language support** forces teams to use multiple build systems
- **Poor incremental builds** mean rebuilding more than necessary

Horcrux solves these problems with intelligent dependency tracking, aggressive caching, parallel execution, and a clean, extensible architecture.

## 💡 Use Cases

Horcrux is designed for diverse build scenarios:

### Large Monorepos

Build massive codebases efficiently with intelligent caching and parallel execution. Perfect for organizations with multi-language monorepos requiring consistent, fast builds.

```bash
# Build entire monorepo in seconds, not minutes
horcrux build //...
```

### Polyglot Projects

Seamlessly build projects mixing C++, Rust, Python, JavaScript, and more — all with a single build system and unified dependency graph.

```bash
# Build frontend, backend, and native modules together
horcrux build //frontend:app //backend:server //native:lib
```

### CI/CD Pipelines

Accelerate continuous integration with hermetic builds, distributed caching, and precise incremental rebuilds. Only rebuild what changed, across all machines.

```bash
# Distributed build with remote cache
horcrux build --remote-cache=redis://cache-server:6379 //...
```

### Local Development

Fast feedback loops with watch mode, incremental compilation, and instant rebuilds. See changes reflected in seconds, not minutes.

```bash
# Watch mode for rapid iteration
horcrux watch //app:dev-server
```

### Cross-Platform Builds

Build for multiple platforms from a single configuration. Consistent builds across Linux, macOS, and Windows with reproducible outputs.

```bash
# Cross-compile for multiple targets
horcrux build --platform=linux-x64,macos-arm64,windows-x64 //app:release
```

## 🧭 Goals

| Goal | Description |
|------|--------------|
| ⚡ **Speed** | 10x faster builds through parallel graph execution and aggressive caching. |
| 🧩 **Universality** | Build any language or tech stack (C++, Python, Java, Go, JS, Rust, etc.). |
| 🧠 **Smart Dependency Graphs** | Predictive, lazy, and memoized dependency resolution. |
| 🧱 **Hermetic Builds** | Fully reproducible builds with isolated sandboxes. |
| 🔍 **Incremental & Correct** | Only rebuild what changed, but never miss a dependency. |
| 🧰 **Extensible Plugins** | Custom toolchains, rules, and integrations using C++ modules or scripts. |
| 🪶 **Lightweight & Portable** | Minimal footprint with zero external runtime dependencies. |

## 🧠 Core Architecture

Horcrux follows a **Hexagonal (Ports & Adapters)** architecture for modularity and testability.

```
+------------------------------------------------------+
| CLI / API Layer                                      |
| (User commands, config parsing, logging)             |
+------------------------------------------------------+
| Build Orchestration                                  |
| Build graph, scheduling, cache manager               |
+------------------------------------------------------+
| Domain Components                                    |
| Rule engine, dependency graph, file watchers, etc.   |
+------------------------------------------------------+
| System Adapters (Infra Layer)                        |
| Filesystem, compilers, network, sandboxing           |
+------------------------------------------------------+
```

Each module will live in its own repository (polyrepo structure), versioned and published to the **Horcrux Central Registry**, allowing reuse and independent evolution.

## 🏗️ Getting Started

### Prerequisites

Ensure you have the following installed:

- **C++23 compiler**: GCC 13+, Clang 17+, or MSVC 2022+
- **CMake 3.25+**
- **Ninja or Make**
- **Git**
- **Linux / macOS / Windows**

### Installation

#### From Source

```bash
# Clone the repository
git clone https://github.com/horcruxsys/horcrux.git
cd horcrux

# Install git hooks (recommended for contributors)
# Hooks automatically format code and run validations before commit/push
lefthook install

# Create and enter build directory
mkdir build && cd build

# Configure the build (Release mode)
cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja

# Build the project (use -j to parallelize)
cmake --build . -- -j$(nproc)

# Run tests to verify the build
ctest --output-on-failure

# Install (optional, requires appropriate permissions)
sudo cmake --install .
```

**Note for contributors:** Git hooks using [Lefthook](https://github.com/evilmartians/lefthook) ensure code quality by:

- Auto-formatting C++ code with clang-format before commit
- Validating YAML files and checking for common issues
- Building and testing before push

See [docs/git-hooks.md](docs/git-hooks.md) for details.

#### Verify Installation

```bash
# Check Horcrux version
horcrux --version

# Run basic health check
horcrux doctor
```

### Quick Start

Create a simple `BUILD` file in your project:

```python
# BUILD file example
cc_binary(
    name = "hello",
    srcs = ["main.cpp"],
)
```

Build and run:

```bash
# Build the target
horcrux build //:hello

# Run the binary
./bazel-bin/hello
```

### Troubleshooting

#### Compiler Not Found

If CMake can't find your C++23 compiler:

```bash
# Specify compiler explicitly
cmake .. -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_BUILD_TYPE=Release
```

#### Build Failures

If the build fails:

1. **Check compiler version**: Ensure you have C++23 support

   ```bash
   g++ --version    # Should be 13.0 or higher
   clang++ --version  # Should be 17.0 or higher
   ```

2. **Clean build directory**: Start fresh

   ```bash
   cd .. && rm -rf build && mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. **Check dependencies**: Ensure CMake and Ninja are up to date

   ```bash
   cmake --version  # Should be 3.25 or higher
   ninja --version
   ```

#### Platform-Specific Issues

**On macOS:**

- Install Xcode Command Line Tools: `xcode-select --install`
- Or use Homebrew: `brew install cmake ninja llvm`

**On Ubuntu/Debian:**

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build g++-13
```

**On Windows:**

- Install Visual Studio 2022 with C++ workload
- Or use MSYS2/MinGW-w64 with GCC 13+

For more help, see our [Contributing Guide](CONTRIBUTING.md) or [open an issue](https://github.com/horcruxsys/horcrux/issues).

## 📖 Usage

### Basic Commands

```bash
# Build a specific target
horcrux build //path/to:target

# Build all targets in a directory
horcrux build //path/to/...

# Run tests
horcrux test //path/to:test_target

# Clean build artifacts
horcrux clean

# Show build graph
horcrux query --graph //path/to:target
```

### Advanced Features

#### Distributed Caching

```bash
# Use remote cache for shared builds
horcrux build --remote-cache=redis://cache.example.com:6379 //...
```

#### Watch Mode

```bash
# Automatically rebuild on file changes
horcrux watch //app:dev-server
```

#### Parallel Builds

```bash
# Control parallelism (default: number of CPU cores)
horcrux build --jobs=8 //...
```

#### Cross-Compilation

```bash
# Build for specific platform
horcrux build --platform=linux-arm64 //app:binary
```

For comprehensive documentation, visit our [documentation site](https://horcruxsys.github.io/horcrux) (coming soon).

## 🧩 Design Guidelines

All modules must follow **Zero Undefined Behavior** and **RAII** principles.

Use `constexpr`, `concepts`, and `std::expected` for safety and clarity.

All operations must be **thread-safe** and **lock-free** wherever possible.

Optimize for **cache locality**, **IO batching**, and **parallel scheduling**.

For detailed coding standards, see our [Contributing Guide](CONTRIBUTING.md#coding-standards).

## 🧪 Benchmark Philosophy

Horcrux defines a **Build Benchmark Suite (HBB)** with automated performance tracking:

- Measures **correctness**, **rebuild time**, **cache efficiency**, and **memory usage**.
- Compares against **Bazel**, **Buck2**, **Ninja**, and **CMake**.
- Benchmarks are **automated** via CI to ensure consistent regression tracking.

### Running Benchmarks

Build and run benchmarks locally:

```bash
# Configure with benchmarks enabled
cmake -B build -DCMAKE_BUILD_TYPE=Release -DHORCRUX_BUILD_BENCHMARKS=ON
cmake --build build

# Run all benchmarks
cd build && cmake --build . --target run_benchmarks

# Or run individual benchmarks
./build/bin/dependency_resolution_bench
./build/bin/cache_lookup_bench
./build/bin/task_scheduling_bench
```

See [benchmarks/README.md](benchmarks/README.md) for detailed documentation.

### Current Metrics

The benchmark suite currently tracks:

- **Dependency Resolution Time**: DAG construction and dependency traversal performance
- **Cache Lookup Latency**: In-memory cache performance with various sizes and patterns
- **Task Scheduling Overhead**: Priority queue operations and task management

Results are automatically collected and posted on pull requests.

## 🗺️ Roadmap

| Milestone | Description | Status |
|-----------|-------------|--------|
| M1 | Create core engine skeleton (BuildEngine) | ✅ |
| M2 | Implement dependency graph & caching layer | ⏳ |
| M3 | Add language adapters (C++, Rust, Python, Java) | ⏳ |
| M4 | Introduce sandboxing and hermetic builds | ⏳ |
| M5 | Integrate plugin and registry system | ⏳ |
| M6 | Release v1.0 stable with public benchmarks | ⏳ |

## 🤝 Contributing

We welcome contributions from everyone! Horcrux is designed for longevity, and we're building a community that values quality, collaboration, and innovation.

### How to Contribute

Contributions should:

- **Maintain predictable performance characteristics**
- **Follow SOLID principles**
- **Include unit + integration tests**
- **Document all public interfaces**

### Getting Started with Contributing

1. Read our [Contributing Guide](CONTRIBUTING.md) for detailed guidelines
2. Check out [good first issues](https://github.com/horcruxsys/horcrux/labels/good-first-issue)
3. Join discussions in [GitHub Discussions](https://github.com/horcruxsys/horcrux/discussions)
4. Review our [Code of Conduct](CODE_OF_CONDUCT.md)

### Ways to Contribute

- 🐛 **Report bugs** - Found an issue? Let us know!
- 💡 **Suggest features** - Have ideas? We want to hear them!
- �� **Improve docs** - Documentation is always appreciated
- 🧪 **Write tests** - Help us maintain quality
- 💻 **Submit code** - Fix bugs or add features
- 🎨 **Design** - UI/UX improvements welcome
- 📣 **Spread the word** - Star, share, and tell others!

## 💬 Getting Help

Need help or have questions? We're here for you!

- 📖 **Documentation**: [Coming soon] - Comprehensive guides and API docs
- 💬 **GitHub Discussions**: [Ask questions and discuss ideas](https://github.com/horcruxsys/horcrux/discussions)
- 🐛 **Issue Tracker**: [Report bugs and request features](https://github.com/horcruxsys/horcrux/issues)
- 📧 **Email**: For private matters, contact [maintainers@horcruxsys.org](mailto:maintainers@horcruxsys.org)
- 💼 **Enterprise Support**: Contact [enterprise@horcruxsys.org](mailto:enterprise@horcruxsys.org)

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for full details.

**Copyright © 2025 Horcrux Project Contributors**

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.**

---

<p align="center">
  <strong>"Builds shouldn't be magic — they should be science."</strong><br>
  — The Horcrux Team
</p>

<p align="center">
  Made with ❤️ by the Horcrux community
</p>
