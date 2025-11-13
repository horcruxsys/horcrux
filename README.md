# 🧱 Horcrux — The Next-Generation Universal Build System

Horcrux is a **modern, high-performance, memory-safe, and developer-friendly build system**, written in **C++23**, designed to build **any project in any language** — from C++ to Rust, Python to Go, and everything in between.

It aims to be **10x faster**, **more deterministic**, and **easier to extend** than existing build systems like **Bazel**, **Buck**, or **CMake**, while maintaining correctness, reproducibility, and minimal resource consumption.

---

## 🚀 Vision

To create the world’s most **intelligent and efficient build system** that understands source code, adapts to developer workflows, and ensures **accurate, reproducible, and incremental builds** — automatically.

Horcrux is built on three foundational principles:

1. **Correctness First** — Builds must always produce consistent, verifiable artifacts.  
2. **Speed by Design** — Every computation, cache, and dependency is optimized for minimal latency.  
3. **Developer Empathy** — Configuration should be intuitive, debuggable, and transparent.

---

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

---

## 🧠 Core Architecture

Horcrux follows a **Hexagonal (Ports & Adapters)** architecture for modularity and testability.

+------------------------------------------------------+
| CLI / API Layer |
| (User commands, config parsing, logging) |
+------------------------------------------------------+
| Build Orchestration |
| Build graph, scheduling, cache manager |
+------------------------------------------------------+
| Domain Components |
| Rule engine, dependency graph, file watchers, etc. |
+------------------------------------------------------+
| System Adapters (Infra Layer) |
| Filesystem, compilers, network, sandboxing |
+------------------------------------------------------+

yaml
Copy code

Each module will live in its own repository (polyrepo structure), versioned and published to the **Horcrux Central Registry**, allowing reuse and independent evolution.

---

## 🏗️ Getting Started

### Requirements
- **C++23 compiler** (GCC 13+, Clang 17+, or MSVC 2022+)
- **CMake 3.25+**
- **Ninja or Make**
- **Linux / macOS / Windows**

### Build & Run
```bash
git clone https://github.com/horcruxsys/horcrux.git
cd horcrux
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j$(nproc)
ctest --output-on-failure
Example Output
bash
Copy code
[Horcrux] Build engine initialized.
Horcrux basic test completed successfully.
🧩 Design Guidelines
All modules must follow Zero Undefined Behavior and RAII principles.

Use constexpr, concepts, and std::expected for safety and clarity.

All operations must be thread-safe and lock-free wherever possible.

Optimize for cache locality, IO batching, and parallel scheduling.

🧪 Benchmark Philosophy
Horcrux will define a Build Benchmark Suite (HBB):

Measures correctness, rebuild time, cache efficiency, and memory usage.

Compares against Bazel, Buck2, Ninja, and CMake.

Benchmarks will be automated to ensure consistent regression tracking.

🤝 Contributing
We’re designing Horcrux for longevity — contributions should:

Maintain predictable performance characteristics

Follow SOLID principles

Include unit + integration tests

Document all public interfaces

🧭 Roadmap
Milestone	Description	Status
M1	Create core engine skeleton (BuildEngine)	✅
M2	Implement dependency graph & caching layer	⏳
M3	Add language adapters (C++, Rust, Python, Java)	⏳
M4	Introduce sandboxing and hermetic builds	⏳
M5	Integrate plugin and registry system	⏳
M6	Release v1.0 stable with public benchmarks	⏳

🧱 License
MIT License © 2025 Horcrux Project Contributors

"Builds shouldn’t be magic — they should be science."
— The Horcrux Team
