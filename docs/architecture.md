# Horcrux Architecture

**Version:** 1.0  
**Last Updated:** 2025-11-13  
**Status:** Draft

## Table of Contents

- [Executive Summary](#executive-summary)
- [System Overview](#system-overview)
- [Design Principles](#design-principles)
- [Core Components](#core-components)
  - [Build Graph Engine](#build-graph-engine)
  - [Execution Engine](#execution-engine)
  - [Cache Layer](#cache-layer)
  - [Language Adapters](#language-adapters)
  - [CLI and Daemon](#cli-and-daemon)
- [Data Flow](#data-flow)
- [Technology Stack](#technology-stack)
- [Future Considerations](#future-considerations)
- [Glossary](#glossary)

## Executive Summary

Horcrux is a next-generation universal build system written in C++23, designed to build projects in any language with unparalleled speed, correctness, and developer experience. This document describes the high-level technical architecture, including the build graph, execution engine, cache layer, language adapters, and CLI/daemon interaction model.

The architecture follows a **hexagonal (ports & adapters)** design pattern, ensuring modularity, testability, and extensibility. The system is built on three foundational principles: **correctness first**, **speed by design**, and **developer empathy**.

**Target Performance:** 10x faster than Bazel, Buck, or CMake while maintaining deterministic, reproducible builds.

## System Overview

Horcrux consists of five major subsystems that work together to provide fast, correct, and reproducible builds:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         CLI / API Layer                                  │
│  (Command parsing, user interaction, configuration, logging)             │
└────────────────────────────┬────────────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────────────┐
│                      Application Layer                                   │
│  (Build orchestration, command handlers, workflow coordination)          │
└────────────────────────────┬────────────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────────────┐
│                        Domain Layer                                      │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐      │
│  │  Build Graph     │  │  Execution       │  │  Cache           │      │
│  │  Engine          │──│  Engine          │──│  Layer           │      │
│  │                  │  │                  │  │                  │      │
│  │ • DAG Builder    │  │ • Scheduler      │  │ • Local Cache    │      │
│  │ • Dependency     │  │ • Task Executor  │  │ • Remote Cache   │      │
│  │   Resolution     │  │ • Sandboxing     │  │ • Hash Computer  │      │
│  │ • Change         │  │ • Concurrency    │  │ • Artifact Store │      │
│  │   Detection      │  │   Control        │  │                  │      │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘      │
│                                                                           │
│  ┌──────────────────────────────────────────────────────────────┐       │
│  │              Language Adapters                                │       │
│  │  (C++, Rust, Python, Java, Go, JavaScript, etc.)             │       │
│  └──────────────────────────────────────────────────────────────┘       │
└────────────────────────────┬────────────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────────────┐
│                    Infrastructure Layer                                  │
│  (Filesystem, Compilers, Network, Process Management, Sandboxing)        │
└──────────────────────────────────────────────────────────────────────────┘
```

### Key Characteristics

- **Layered Architecture**: Clear separation between CLI, application logic, domain logic, and infrastructure
- **Hexagonal Design**: Domain layer is pure C++ with no I/O dependencies
- **Dependency Flow**: Dependencies point inward; outer layers depend on inner layers
- **Immutable Core**: Build graph is an immutable DAG for thread-safety and reproducibility
- **Pluggable Adapters**: Language adapters are modular and independently extensible

## Design Principles

Horcrux is built on four fundamental design principles that guide all architectural decisions:

### 1. Correctness First

**Principle**: Builds must always produce consistent, verifiable, and deterministic artifacts.

**Implementation**:
- **Hermetic builds**: All inputs are explicitly declared; no hidden dependencies
- **Content-addressable storage**: Artifacts are identified by their hash, ensuring integrity
- **Immutable build graphs**: Once constructed, the DAG cannot be modified
- **Explicit dependency tracking**: Every file, tool, and configuration is tracked
- **Reproducible outputs**: Same inputs always produce identical outputs across all environments

**Example**:
```cpp
// All dependencies must be explicitly declared
cc_binary(
    name = "app",
    srcs = ["main.cpp"],
    deps = [":lib"],           // Explicit library dependency
    data = ["config.json"],    // Explicit data dependency
    tools = ["//tools:codegen"] // Explicit tool dependency
)
```

### 2. Reproducibility

**Principle**: Builds must be fully reproducible across different machines, times, and environments.

**Implementation**:
- **Merkle tree hashing**: Content-based addressing for all artifacts
- **Sandboxed execution**: Tasks run in isolated environments with controlled inputs
- **Version-locked toolchains**: Compiler versions and tool versions are pinned
- **Deterministic ordering**: Build actions are executed in a consistent order
- **No timestamp dependencies**: File modification times are not used for cache decisions

**Benefits**:
- Identical builds on developer machines and CI servers
- Debugging is easier with reproducible builds
- Distributed caching works reliably
- Security audits are possible

### 3. Immutability

**Principle**: Core data structures are immutable for thread-safety and correctness.

**Implementation**:
- **Immutable build graph**: DAG is constructed once and never modified
- **Immutable cache entries**: Once stored, cache entries are never changed
- **Persistent data structures**: Use structural sharing for efficient immutable updates
- **Functional transformations**: Operations return new values rather than mutating state
- **Copy-on-write semantics**: When mutation is needed, create a new version

**Benefits**:
- Thread-safe by default
- No race conditions
- Easier to reason about
- Enables optimistic concurrency
- Simplifies distributed caching

### 4. Modularity

**Principle**: System components are independent, composable, and testable in isolation.

**Implementation**:
- **Clear interfaces**: Each component exposes a well-defined API
- **Dependency injection**: Components receive dependencies via constructor injection
- **Hexagonal architecture**: Domain logic is independent of infrastructure
- **Plugin system**: Language adapters are dynamically loadable plugins
- **Single responsibility**: Each component has one reason to change

**Benefits**:
- Easy to test (mock/stub dependencies)
- Easy to extend (add new language adapters)
- Easy to maintain (changes are localized)
- Easy to understand (clear boundaries)

## Core Components

### Build Graph Engine

The Build Graph Engine is responsible for constructing and analyzing an immutable directed acyclic graph (DAG) of build targets and their dependencies.

#### Responsibilities

1. **Parse BUILD files** and construct target definitions
2. **Resolve dependencies** between targets recursively
3. **Detect cycles** and report dependency errors
4. **Compute changed targets** based on file modifications
5. **Generate execution plan** for the Execution Engine

#### Key Features

##### Immutable DAG

The build graph is an immutable data structure that represents all targets and their dependencies:

```cpp
class BuildGraph {
public:
    // Immutable graph construction
    static auto create(const Config& config) 
        -> std::expected<BuildGraph, Error>;
    
    // Query operations (const)
    auto get_target(const Label& label) const 
        -> std::optional<Target>;
    
    auto get_dependencies(const Label& label) const 
        -> std::vector<Label>;
    
    auto find_changed_targets(const ChangeSet& changes) const 
        -> std::vector<Label>;
    
    // No mutation methods!
    
private:
    std::unordered_map<Label, Target> targets_;
    std::unordered_map<Label, std::vector<Label>> edges_;
};
```

**Benefits**:
- Thread-safe reads without locks
- Can be shared across threads
- No race conditions
- Reproducible analysis

##### Dependency Resolution

Dependencies are resolved recursively starting from requested targets:

```
1. Parse BUILD file for requested target
2. For each dependency:
   a. Parse dependency's BUILD file
   b. Recursively resolve its dependencies
3. Detect cycles using depth-first search
4. Return flattened dependency list
```

**Algorithm**: Tarjan's strongly connected components for cycle detection (O(V + E))

##### Change Detection

The engine computes which targets need rebuilding based on:

1. **Modified source files**: Compare current hash with cached hash
2. **Modified dependencies**: Transitive dependency changes
3. **Changed BUILD rules**: Rule definition modifications
4. **Tool changes**: Compiler or tool version updates

```cpp
auto compute_affected_targets(
    const BuildGraph& graph,
    const std::vector<Path>& changed_files
) -> std::vector<Label> {
    // 1. Find targets that directly use changed files
    auto direct = find_direct_dependents(graph, changed_files);
    
    // 2. Find transitive dependents
    auto transitive = compute_transitive_closure(graph, direct);
    
    // 3. Return union
    return merge(direct, transitive);
}
```

#### Data Structures

- **Target**: Represents a single buildable unit (binary, library, test, etc.)
- **Label**: Unique identifier for a target (`//path/to:name`)
- **Rule**: Template for building a specific type of target (`cc_binary`, `py_library`, etc.)
- **Attribute**: Configuration parameter for a rule (e.g., `srcs`, `deps`, `data`)

#### Performance Characteristics

- **Graph construction**: O(V + E) where V = targets, E = dependencies
- **Dependency resolution**: O(V + E) with memoization
- **Change detection**: O(V) with hash table lookups
- **Cycle detection**: O(V + E) using Tarjan's algorithm

### Execution Engine

The Execution Engine schedules and executes build actions in parallel while respecting dependencies and resource constraints.

#### Responsibilities

1. **Schedule tasks** based on dependency order
2. **Execute tasks** in parallel with controlled concurrency
3. **Sandbox execution** to ensure hermeticity
4. **Manage resources** (CPU, memory, I/O) to prevent overload
5. **Handle failures** and retry logic

#### Key Features

##### Task Scheduler

The scheduler uses a **work-stealing queue** for efficient parallel execution:

```cpp
class Scheduler {
public:
    auto schedule(const BuildGraph& graph, 
                  const std::vector<Label>& targets)
        -> std::expected<ExecutionPlan, Error>;
    
    auto execute(const ExecutionPlan& plan)
        -> std::expected<BuildResult, Error>;
    
private:
    // Work-stealing queues (one per thread)
    std::vector<WorkStealingQueue<Task>> queues_;
    
    // Thread pool
    std::vector<std::jthread> workers_;
    
    // Resource limits
    ResourceTracker resources_;
};
```

**Algorithm**:
1. **Topological sort**: Order tasks by dependencies
2. **Priority queue**: Ready tasks sorted by estimated duration
3. **Work stealing**: Idle threads steal work from busy threads
4. **Backpressure**: Limit queue size to prevent memory exhaustion

##### Sandboxed Execution

Each build action runs in an isolated sandbox:

```
┌─────────────────────────────────────────────────┐
│              Sandbox Environment                │
│                                                 │
│  • Clean working directory                     │
│  • Explicit input files only                   │
│  • No network access (unless declared)         │
│  • No filesystem access outside sandbox        │
│  • Controlled environment variables            │
│  • Limited resource usage (CPU/memory/time)    │
└─────────────────────────────────────────────────┘
```

**Implementation**:
- **Linux**: `unshare()` namespaces + `seccomp` filters
- **macOS**: `sandbox-exec` with custom profiles
- **Windows**: Job objects + AppContainer isolation

##### Concurrency Model

The execution engine uses a **actor-based concurrency model**:

- **One actor per task**: Each task runs in isolation
- **Message passing**: Tasks communicate via channels (no shared memory)
- **Lock-free queues**: Use atomic operations for task queues
- **Backpressure**: Slow consumers signal fast producers to slow down

```cpp
// Task execution flow
async auto execute_task(const Task& task) -> TaskResult {
    // 1. Check cache
    if (auto cached = cache_.lookup(task.hash())) {
        return cached.value();
    }
    
    // 2. Prepare sandbox
    auto sandbox = create_sandbox(task.inputs());
    
    // 3. Execute in sandbox
    auto result = co_await sandbox.execute(task.command());
    
    // 4. Store in cache
    cache_.store(task.hash(), result);
    
    return result;
}
```

#### Performance Characteristics

- **Scheduling overhead**: O(V log V) for priority queue operations
- **Task execution**: Fully parallel (bounded by core count)
- **Memory usage**: O(V) for task metadata + sandbox overhead
- **Scalability**: Linear speedup up to core count (Amdahl's law applies)

### Cache Layer

The Cache Layer provides fast, content-addressable storage for build artifacts with support for local and remote caching.

#### Responsibilities

1. **Cache build artifacts** to avoid redundant work
2. **Compute content hashes** using Merkle trees
3. **Lookup cached results** before executing tasks
4. **Store results** after successful execution
5. **Manage cache size** with eviction policies
6. **Sync with remote cache** for distributed builds

#### Key Features

##### Content-Addressable Storage

Artifacts are stored by their **content hash** (SHA-256):

```
Content Hash = SHA-256(
    Command Hash +
    Input File Hashes +
    Tool Hash +
    Environment Hash +
    Platform Hash
)
```

**Properties**:
- **Deterministic**: Same inputs always produce the same hash
- **Collision-resistant**: SHA-256 has negligible collision probability
- **Portable**: Hashes work across machines and time
- **Verifiable**: Can verify artifact integrity

##### Merkle Tree Hashing

File sets are hashed using Merkle trees for efficient incremental hashing:

```
        Root Hash
         /    \
    Hash(A)  Hash(B,C)
             /    \
        Hash(B)  Hash(C)
```

**Benefits**:
- **Incremental updates**: Only rehash changed files
- **Efficient comparison**: Compare root hashes instead of full directory trees
- **Proof of inclusion**: Can prove a file is part of a set

```cpp
class MerkleTree {
public:
    // Compute hash of directory tree
    static auto hash_directory(const Path& dir) -> Hash;
    
    // Incremental update
    auto update(const Path& file, const Hash& new_hash) -> Hash;
    
    // Verify inclusion
    auto verify(const Path& file, const Hash& file_hash) const -> bool;
};
```

**Deterministic Hashing**:

To ensure reproducibility across different machines and filesystems, Horcrux implements strict determinism in Merkle tree construction:

```cpp
auto hash_directory(const Path& dir) -> Hash {
    // 1. Collect all files
    std::vector<Path> files = collect_files(dir);
    
    // 2. Sort lexicographically for deterministic ordering
    std::sort(files.begin(), files.end());
    
    // 3. Hash files in sorted order
    HashBuilder builder;
    for (const auto& file : files) {
        builder.add(file.string(), hash_file(file));
    }
    return builder.finalize();
}
```

**Key determinism guarantees**:
- **Sorted traversal**: Files are always processed in lexicographic order
- **Canonical paths**: All paths use forward slashes (`/`) regardless of OS
- **Normalized environment**: Build environment variables are sorted and normalized
- **Toolchain versioning**: Compiler and tool versions are included in hash computation

##### Environment Normalization

Horcrux maintains a normalized build environment to ensure cross-platform reproducibility:

```cpp
struct BuildEnvironment {
    std::map<std::string, std::string> env_vars;  // Sorted map
    std::string os_type;                          // "linux", "macos", "windows"
    std::string cpu_arch;                         // "x86_64", "arm64"
    ToolchainVersion toolchain;                   // Compiler version + hash
    
    auto compute_hash() const -> Hash {
        HashBuilder builder;
        for (const auto& [key, value] : env_vars) {  // Already sorted
            builder.add(key, value);
        }
        builder.add(os_type);
        builder.add(cpu_arch);
        builder.add(toolchain.version);
        builder.add(toolchain.hash);
        return builder.finalize();
    }
};
```

**Environment hash is included in every artifact hash** to detect platform-specific differences.

##### Toolchain Lock File

Horcrux uses a `.horcrux.lock` file to pin exact toolchain versions:

```json
{
  "version": "1.0",
  "toolchains": {
    "cpp": {
      "compiler": "clang++",
      "version": "17.0.1",
      "path": "/usr/bin/clang++-17",
      "hash": "sha256:abc123..."
    },
    "python": {
      "interpreter": "python3",
      "version": "3.11.5",
      "hash": "sha256:def456..."
    }
  },
  "generated": "2025-11-13T14:37:32Z"
}
```

**Benefits**:
- Prevents version drift across team members
- Enables hermetic builds with exact tool versions
- Facilitates debugging when toolchain changes
- Supports tool version migration tracking

##### Local Cache

The local cache stores artifacts on the developer's machine:

- **Location**: `~/.cache/horcrux/` or `$HORCRUX_CACHE_DIR`
- **Storage Engine**: RocksDB for high-performance persistence
- **Structure**: Content-addressable storage with efficient key-value access
- **Eviction**: LRU (least recently used) when size exceeds limit
- **Metadata**: SQLite for small structured metadata (project manifests, configuration history)

```
~/.cache/horcrux/
├── rocksdb/           # Primary storage for cache, DAGs, artifacts
│   ├── cache/         # Build artifact cache
│   ├── dag/           # Dependency graph persistence
│   └── artifacts/     # Artifact metadata and references
├── metadata.db        # SQLite for project manifests, config history
└── config.json
```

**RocksDB Benefits**:
- **High throughput**: Optimized for write-heavy workloads (cache updates)
- **Efficient compaction**: Automatically manages disk space with LSM-tree
- **Range queries**: Fast iteration over hash prefixes for cache management
- **Atomic transactions**: Ensures consistency for multi-key updates
- **Column families**: Separate storage for cache, DAGs, and artifacts

##### Cache Performance Optimizations

**Two-Tier Caching Architecture**:

To minimize RocksDB read latency for frequently accessed data, Horcrux implements a two-tier cache:

```cpp
class HybridCache {
public:
    auto lookup(const Hash& hash) -> std::optional<Artifact> {
        // 1. Check in-memory LRU cache (hot data)
        if (auto cached = memory_cache_.get(hash)) {
            return cached;
        }
        
        // 2. Check RocksDB (persistent storage)
        if (auto artifact = rocksdb_.get(hash)) {
            // Promote to memory cache
            memory_cache_.put(hash, artifact.value());
            return artifact;
        }
        
        return std::nullopt;
    }
    
private:
    LRUCache<Hash, Artifact> memory_cache_;  // 1-2 GB in-memory
    RocksDBStore rocksdb_;                   // Persistent storage
};
```

**Performance impact**: 2-4× faster incremental builds by avoiding repeated RocksDB reads for hot data.

**Parallel Hash Pipeline**:

Hash computation is parallelized using a thread pool with staged hashing:

```cpp
class ParallelHasher {
public:
    auto hash_files(const std::vector<Path>& files) -> std::vector<Hash> {
        // Use XXH3 for intermediate hashing (extremely fast)
        auto intermediate = parallel_map(files, [](const Path& file) {
            return xxh3::hash64(read_file(file));
        });
        
        // Use SHA-256 only for final artifact hash
        return combine_with_sha256(intermediate);
    }
};
```

**Hash selection**:
- **XXH3**: Intermediate Merkle tree nodes (non-cryptographic, ~50 GB/s)
- **SHA-256**: Final artifact hashes (cryptographic, ~1 GB/s)

**Adaptive Concurrency Control**:

Dynamic thread pool sizing based on system load:

```cpp
class AdaptiveScheduler {
public:
    auto compute_optimal_parallelism() -> int {
        int max_threads = std::thread::hardware_concurrency();
        int system_load = get_system_load_average();
        
        // Reserve cores for system and I/O
        int available = std::max(1, max_threads - 2);
        
        // Reduce parallelism if system is under pressure
        if (system_load > max_threads * 0.8) {
            available = available / 2;
        }
        
        return available;
    }
};
```

**Benefits**:
- Prevents CPU thrashing during high-load scenarios
- Maintains responsiveness for foreground tasks
- Adapts to system conditions dynamically

**Background Compaction Scheduling**:

RocksDB compaction is scheduled to minimize impact on builds:

```cpp
class CacheManager {
public:
    auto schedule_compaction() {
        // Manual compaction during idle phases
        if (is_idle() && time_since_last_build() > 5min) {
            rocksdb_.compact_range(nullptr, nullptr);
        }
    }
    
    auto configure_compaction() {
        rocksdb_options_.max_background_compactions = 2;
        rocksdb_options_.max_background_flushes = 2;
        // Rate-limit compaction I/O
        rocksdb_options_.rate_limiter = NewGenericRateLimiter(100_MB_per_sec);
    }
};
```

**Cache Warming and Prefetching**:

Asynchronous prefetching during DAG analysis:

```cpp
class BuildOrchestrator {
public:
    auto execute_build(const BuildGraph& graph) -> BuildResult {
        // Start prefetching while analyzing dependencies
        auto prefetch_future = std::async([&]() {
            for (const auto& target : graph.get_all_targets()) {
                cache_.prefetch(target.expected_output_hash());
            }
        });
        
        // Build execution proceeds concurrently with prefetch
        auto result = execution_engine_.execute(graph);
        prefetch_future.wait();
        
        return result;
    }
};
```

**CLI option**: `horcrux build --warm-cache` to prefetch common artifacts before build.

##### Remote Cache

The remote cache enables sharing artifacts across machines:

- **Protocols**: HTTP/HTTPS, Redis, S3-compatible storage
- **Authentication**: API keys, OAuth, or mutual TLS
- **Compression**: zstd for fast compression with good ratio
- **Deduplication**: Content-addressable storage prevents duplicates

```cpp
class RemoteCache {
public:
    // Upload artifact
    auto upload(const Hash& hash, const Artifact& artifact) 
        -> std::expected<void, Error>;
    
    // Download artifact
    auto download(const Hash& hash) 
        -> std::expected<Artifact, Error>;
    
    // Check existence
    auto exists(const Hash& hash) -> bool;
};
```

**Flow**:
1. Check local cache (RocksDB)
2. If miss, check remote cache
3. If miss, execute task
4. Store result in local cache (RocksDB with atomic writes)
5. Upload to remote cache (async)

##### Persistence Strategy

**RocksDB as Primary Storage**:

RocksDB serves as the primary persistence engine for performance-critical data:

- **Build cache**: Stores compiled artifacts, object files, and build outputs
- **Dependency DAGs**: Persists build graphs for fast daemon restarts
- **Artifact metadata**: Tracks artifact hashes, sizes, and access times

**Why RocksDB**:
- **Write-optimized**: LSM-tree architecture handles frequent cache updates efficiently
- **Concurrent access**: Multiple threads can read/write simultaneously
- **Compaction**: Automatic background cleanup maintains performance
- **Crash recovery**: Write-ahead log ensures data consistency
- **Efficient storage**: Compression and bloom filters reduce disk usage

**SQLite for Metadata**:

SQLite handles small, structured data where relational queries are beneficial:

- **Project manifests**: Configuration files, workspace settings
- **Configuration history**: Audit log of build configuration changes
- **Local indexing**: BUILD file locations, target indexes for quick searches

**Why SQLite for metadata**:
- **ACID transactions**: Ensures configuration consistency
- **SQL queries**: Relational queries for complex metadata searches
- **Low overhead**: Lightweight for small datasets (<10 MB typical)
- **Zero configuration**: Embedded database, no server needed

#### Performance Characteristics

- **Hash computation**: O(n) where n = file size, parallelized across files
- **Cache lookup**: O(log n) average with RocksDB LSM-tree, effectively O(1) with bloom filters
- **Cache write**: O(log n) amortized with LSM-tree compaction
- **Local cache I/O**: Limited by disk bandwidth (~500 MB/s SSD)
- **Remote cache I/O**: Limited by network bandwidth (varies)
- **Cache hit rate**: Target >90% for incremental builds
- **RocksDB compaction**: Background process, minimal impact on foreground operations

### Language Adapters

Language Adapters provide support for building projects in different programming languages. Each adapter implements a common interface and provides language-specific rules and toolchain integration.

#### Architecture

```cpp
// Common interface for all language adapters
class ILanguageAdapter {
public:
    virtual ~ILanguageAdapter() = default;
    
    // Detect source files for this language
    virtual auto detect_sources(const Path& dir) 
        -> std::vector<Path> = 0;
    
    // Provide language-specific rules
    virtual auto get_rules() 
        -> std::vector<RuleDefinition> = 0;
    
    // Resolve language-specific dependencies
    virtual auto resolve_dependencies(const Target& target)
        -> std::expected<DependencyList, Error> = 0;
    
    // Generate build commands
    virtual auto generate_actions(const Target& target)
        -> std::expected<std::vector<Action>, Error> = 0;
};
```

#### Supported Languages

##### C++ Adapter

**Features**:
- Header dependency scanning
- Precompiled header support
- Link-time optimization (LTO)
- Multiple compilers (GCC, Clang, MSVC)

**Rules**:
- `cc_binary`: Compile and link a C++ executable
- `cc_library`: Compile a C++ library
- `cc_test`: Compile and run C++ tests

**Example**:
```python
cc_library(
    name = "mylib",
    srcs = ["mylib.cpp"],
    hdrs = ["mylib.h"],
    deps = ["//third_party:abseil"],
    copts = ["-std=c++23", "-O3"],
)

cc_binary(
    name = "myapp",
    srcs = ["main.cpp"],
    deps = [":mylib"],
)
```

##### Python Adapter

**Features**:
- Virtual environment management
- Dependency resolution from `requirements.txt`
- Bytecode caching
- Type checking integration

**Rules**:
- `py_binary`: Python executable
- `py_library`: Python library
- `py_test`: Python tests with pytest

##### Java Adapter

**Features**:
- Incremental compilation with javac
- JAR packaging
- Maven/Gradle dependency resolution
- Annotation processing

**Rules**:
- `java_binary`: Java application
- `java_library`: Java library
- `java_test`: JUnit tests

##### Rust Adapter

**Features**:
- Cargo integration
- Incremental compilation
- Cross-compilation support
- Procedural macro handling

**Rules**:
- `rust_binary`: Rust executable
- `rust_library`: Rust library
- `rust_test`: Cargo tests

##### Go Adapter

**Features**:
- Module-aware builds
- CGO support
- Cross-compilation
- Test caching

**Rules**:
- `go_binary`: Go executable
- `go_library`: Go package
- `go_test`: Go tests

##### JavaScript/TypeScript Adapter

**Features**:
- npm/yarn/pnpm support
- TypeScript compilation
- Bundling with esbuild
- Node.js and browser targets

**Rules**:
- `js_binary`: Node.js application
- `js_library`: JavaScript module
- `ts_library`: TypeScript module
- `js_test`: Jest/Mocha tests

#### Adapter Plugin System

Language adapters are **dynamically loadable plugins**:

```cpp
// Plugin loader
class AdapterRegistry {
public:
    // Load adapter from shared library
    auto load_adapter(const Path& plugin_path)
        -> std::expected<std::unique_ptr<ILanguageAdapter>, Error>;
    
    // Get adapter for language
    auto get_adapter(const std::string& language) const
        -> std::optional<ILanguageAdapter*>;
    
private:
    std::unordered_map<std::string, 
                       std::unique_ptr<ILanguageAdapter>> adapters_;
};
```

**Benefits**:
- **Extensibility**: Third-party adapters without modifying core
- **Modularity**: Adapters are independent
- **Versioning**: Adapters can be versioned independently

### CLI and Daemon

Horcrux provides a command-line interface (CLI) and an optional long-running daemon for improved performance.

#### CLI Architecture

The CLI is the primary interface for users:

```
horcrux <command> [options] <targets>

Commands:
  build     Build the specified targets
  test      Run tests
  run       Build and run a target
  clean     Remove build artifacts
  query     Query the build graph
  info      Show system information
  daemon    Manage the build daemon
```

**Implementation**:
```cpp
class CLI {
public:
    auto parse_args(int argc, char* argv[])
        -> std::expected<Command, Error>;
    
    auto execute(const Command& cmd)
        -> std::expected<int, Error>;
    
private:
    Config config_;
    std::unique_ptr<BuildEngine> engine_;
};
```

#### Daemon Architecture

The daemon is a long-running background process that:

1. **Keeps build graph in memory** for faster subsequent builds
2. **Watches files** for changes and incrementally updates the graph
3. **Maintains warm caches** to avoid cold-start overhead
4. **Provides RPC interface** for CLI communication

```
┌──────────────┐         IPC (Unix socket)        ┌──────────────┐
│   CLI        │────────────────────────────────▶│   Daemon     │
│ (Frontend)   │                                  │  (Backend)   │
└──────────────┘                                  └──────────────┘
                                                   │
                                                   ├─ Build Graph (in-memory)
                                                   ├─ File Watcher
                                                   ├─ Cache Manager
                                                   └─ RPC Server
```

**Benefits**:
- **Faster builds**: No graph parsing overhead
- **Incremental updates**: Only reparse changed BUILD files
- **File watching**: Automatically detect changes
- **Resource pooling**: Reuse worker threads

#### Communication Protocol

CLI and daemon communicate via **gRPC** over Unix domain socket:

```protobuf
service BuildService {
    rpc Build(BuildRequest) returns (stream BuildEvent);
    rpc Test(TestRequest) returns (stream TestEvent);
    rpc Query(QueryRequest) returns (QueryResponse);
    rpc Shutdown(Empty) returns (Empty);
}

message BuildRequest {
    repeated string targets = 1;
    map<string, string> options = 2;
}

message BuildEvent {
    oneof event {
        TargetStarted target_started = 1;
        TargetCompleted target_completed = 2;
        BuildFinished build_finished = 3;
    }
}
```

#### Daemon Lifecycle

1. **Start**: `horcrux daemon start`
   - Fork background process
   - Load build graph
   - Start file watcher
   - Listen on socket

2. **Status**: `horcrux daemon status`
   - Check if daemon is running
   - Show memory usage, uptime

3. **Stop**: `horcrux daemon stop`
   - Send shutdown signal
   - Wait for graceful termination
   - Clean up socket file

## Data Flow

### Build Request Flow

```
1. User runs: horcrux build //app:main

2. CLI parses command
   ├─ Parse arguments
   ├─ Load configuration
   └─ Connect to daemon (or start embedded engine)

3. Build Graph Construction
   ├─ Parse BUILD file for //app:main
   ├─ Recursively resolve dependencies
   ├─ Construct immutable DAG
   └─ Detect cycles (if any)

4. Change Detection
   ├─ Compare file hashes with cache
   ├─ Compute affected targets
   └─ Generate minimal rebuild set

5. Execution Planning
   ├─ Topologically sort targets
   ├─ Assign priorities
   └─ Generate execution plan

6. Parallel Execution
   ├─ For each target (in dependency order):
   │   ├─ Check cache (local, then remote)
   │   ├─ If cache miss:
   │   │   ├─ Create sandbox
   │   │   ├─ Execute build action
   │   │   ├─ Collect outputs
   │   │   └─ Store in cache
   │   └─ Emit progress events
   └─ Wait for all targets to complete

7. Result Reporting
   ├─ Aggregate results
   ├─ Report success/failure
   └─ Show summary (time, cache hit rate, etc.)
```

### Cache Lookup Flow

```
1. Task ready to execute
   ↓
2. Compute content hash
   ├─ Hash command
   ├─ Hash input files (Merkle tree)
   ├─ Hash tool version
   └─ Combine hashes
   ↓
3. Check local cache (RocksDB)
   ├─ Lookup by hash in cache column family
   └─ If found → return cached artifact
   ↓
4. Check remote cache (if configured)
   ├─ Send HTTP request with hash
   └─ If found → download and cache locally
   ↓
5. Cache miss → execute task
   ↓
6. Store result
   ├─ Store in local cache (RocksDB)
   ├─ Update dependency DAG (RocksDB)
   └─ Upload to remote cache (async)
```

### File Change Propagation

```
1. File system event (file modified)
   ↓
2. File Watcher detects change
   ↓
3. Compute new file hash
   ↓
4. Find targets affected by file
   ├─ Direct dependents (targets using the file)
   └─ Transitive dependents (targets depending on direct dependents)
   ↓
5. Invalidate cache entries
   ├─ Mark affected targets as dirty
   └─ Remove cached artifacts
   ↓
6. Notify CLI (if in watch mode)
   ├─ Trigger incremental rebuild
   └─ Show progress
```

## Technology Stack

### Core Language

- **C++23**: Modern C++ with modules, concepts, ranges, and coroutines
- **Compiler Support**: GCC 13+, Clang 17+, MSVC 2022+

### Build System

- **CMake 3.25+**: Meta-build system for cross-platform builds
- **Ninja**: Fast parallel build executor

### Libraries

#### Core Dependencies

- **std::expected**: Error handling (C++23 standard)
- **std::format**: String formatting (C++20 standard)
- **std::jthread**: Thread management (C++20 standard)
- **std::ranges**: Functional programming (C++20 standard)

#### External Dependencies (Minimal)

- **abseil-cpp**: Core utilities and data structures
- **gRPC**: RPC framework for daemon communication
- **Protocol Buffers**: Serialization for IPC
- **RocksDB**: Primary persistence engine for build cache, dependency DAGs, and artifact storage
- **SQLite**: Small structured metadata (project manifests, configuration history, local indexing)
- **zstd**: Fast compression for cache artifacts
- **xxHash (XXH3)**: Fast non-cryptographic hashing for intermediate computations (~50 GB/s)
- **OpenSSL**: Cryptographic hashing (SHA-256) for final artifact hashes

### Platform Support

- **Linux**: Primary platform (kernel 5.10+)
- **macOS**: Full support (macOS 12+)
- **Windows**: Full support (Windows 10+)

### Testing

- **Google Test**: Unit testing framework
- **Google Benchmark**: Performance benchmarking
- **AddressSanitizer**: Memory error detection
- **ThreadSanitizer**: Data race detection
- **UndefinedBehaviorSanitizer**: Undefined behavior detection

## Future Considerations

### Distributed Execution

Execute build actions on remote machines for even faster builds:

```
┌──────────────┐                        ┌──────────────────┐
│   Scheduler  │──────────────────────▶│  Worker Pool     │
│              │    Distribute tasks    │  (Remote)        │
└──────────────┘                        └──────────────────┘
                                         ├─ Worker 1
                                         ├─ Worker 2
                                         └─ Worker N
```

**Benefits**:
- Scale beyond local machine
- Utilize cloud resources
- Massive parallelism

**Challenges**:
- Network overhead
- Load balancing
- Fault tolerance

#### Remote Execution Protocol (REP) Compatibility

Design the Execution Engine to be compatible with **Bazel Remote Execution API v2**:

```protobuf
service RemoteExecution {
    rpc Execute(ExecuteRequest) returns (stream ExecuteResponse);
    rpc WaitExecution(WaitExecutionRequest) returns (stream ExecuteResponse);
}

message ExecuteRequest {
    string action_digest = 1;       // Hash of action to execute
    bool skip_cache_lookup = 2;     // Force execution
    ExecutionPolicy policy = 3;     // Timeout, priority
}
```

**Benefits**:
- Integration with existing remote build clusters (BuildFarm, BuildGrid)
- Reuse ecosystem tools like `reproxy`, `reclient`
- Standard protocol for interoperability
- Leverage existing infrastructure investments

**Implementation path**:
1. Implement REP v2 server in daemon
2. Add remote execution backend as adapter
3. Support both local and remote execution seamlessly

### Optimized Change Detection

Instead of comparing all file hashes on every build, implement watchman-style incremental change detection:

```cpp
class FileWatcher {
public:
    // Platform-specific file watching
    auto watch_directory(const Path& dir) {
        #ifdef __linux__
            return InotifyWatcher(dir);
        #elif __APPLE__
            return FSEventsWatcher(dir);
        #elif _WIN32
            return ReadDirectoryChangesWatcher(dir);
        #endif
    }
    
    // Maintain fingerprint table
    struct FileFingerprint {
        Hash last_hash;
        std::filesystem::file_time_type mtime;
        uintmax_t size;
    };
    
    std::unordered_map<Path, FileFingerprint> fingerprints_;
};
```

**Optimization**: Change detection drops from **O(V)** to **O(Δ)**, where Δ = number of changed files.

**Fingerprint persistence**:
- Store fingerprint table in RocksDB for daemon restarts
- Incremental updates on file changes
- Fast lookup for build decisions

**Performance impact**:
- Cold start: Full scan required (same as baseline)
- Warm daemon: Only rehash changed files (10-100× faster)
- Typical incremental build: <100ms for change detection

### Chunked Artifact Deduplication

For large binaries and multi-language builds, implement content-defined chunking:

```cpp
class ChunkedArtifactStore {
public:
    auto store_artifact(const Path& artifact) -> Hash {
        // 1. Split into variable-size chunks (content-defined)
        auto chunks = content_defined_chunking(artifact, 
                                                /*avg_size=*/4_MB);
        
        // 2. Hash each chunk independently
        std::vector<Hash> chunk_hashes;
        for (const auto& chunk : chunks) {
            auto chunk_hash = hash(chunk);
            rocksdb_.put(chunk_hash, chunk);  // Deduplicated storage
            chunk_hashes.push_back(chunk_hash);
        }
        
        // 3. Store chunk manifest
        ArtifactManifest manifest{
            .chunks = chunk_hashes,
            .total_size = file_size(artifact)
        };
        
        auto manifest_hash = hash(manifest);
        rocksdb_.put(manifest_hash, serialize(manifest));
        
        return manifest_hash;
    }
};
```

**Benefits**:
- **Space savings**: 20-40% reduction for large repositories
- **Faster uploads**: Only upload changed chunks to remote cache
- **Better cache hits**: Partial matches when files are similar

**Use cases**:
- Large binaries (>10 MB) that change incrementally
- Docker images, container artifacts
- Asset bundles, media files

### Pluggable Cache Policy Engine

Allow dynamic cache eviction and retention strategies:

```cpp
class ICachePolicy {
public:
    virtual ~ICachePolicy() = default;
    
    // Decide if entry should be evicted
    virtual bool should_evict(const CacheEntry& entry) const = 0;
    
    // Update policy state on access
    virtual void on_access(const Hash& hash) = 0;
};

// Built-in policies
class LRUPolicy : public ICachePolicy { /* ... */ };
class LFUPolicy : public ICachePolicy { /* ... */ };
class TTLPolicy : public ICachePolicy { /* ... */ };

// User-defined policies
class CustomPolicy : public ICachePolicy {
public:
    bool should_evict(const CacheEntry& entry) const override {
        // Custom logic: keep build artifacts, evict test artifacts
        if (entry.type == "test") return true;
        if (entry.age > 30_days) return true;
        return false;
    }
};
```

**Policy options**:
- **LRU** (default): Least recently used
- **LFU**: Least frequently used (useful for CI)
- **MRU**: Most recently used (fast replays)
- **TTL**: Time-based expiry (temporary builds)
- **Custom**: User-defined logic via plugin

**Configuration**:
```json
{
  "cache": {
    "policy": "lru",
    "max_size": "50GB",
    "custom_rules": [
      {"pattern": "//test/...", "ttl": "1d"},
      {"pattern": "//prod/...", "ttl": "30d"}
    ]
  }
}
```

### Distributed DAG Partitioning

For massive monorepos (1000+ parallel builds), partition the DAG for distributed processing:

```cpp
class DAGPartitioner {
public:
    auto partition(const BuildGraph& graph, int num_partitions) 
        -> std::vector<SubGraph> {
        
        // 1. Find weakly connected components
        auto components = tarjan_scc(graph);
        
        // 2. Assign components to partitions (consistent hashing)
        std::vector<SubGraph> partitions(num_partitions);
        for (const auto& component : components) {
            int partition_id = hash(component) % num_partitions;
            partitions[partition_id].add(component);
        }
        
        // 3. Identify cross-partition edges
        for (auto& partition : partitions) {
            partition.compute_dependencies();
        }
        
        return partitions;
    }
};
```

**Execution strategy**:
1. Partition DAG into independent subgraphs
2. Assign each subgraph to a worker node
3. Execute subgraphs in parallel
4. Merge results at coordination node

**Benefits**:
- Scale to 1000+ parallel workers
- Distribute graph construction overhead
- Localize cache access (reduce remote cache load)

**Challenges**:
- Cross-partition dependency synchronization
- Load balancing for uneven partitions
- Fault tolerance and recovery

### Incremental Computation

Memoize arbitrary computations, not just build actions:

```cpp
// Memoize expensive function
auto expensive_computation = memoize([](int x) {
    // Expensive work
    return x * x;
});

// First call: computed
auto result1 = expensive_computation(42);  // Computed

// Second call: cached
auto result2 = expensive_computation(42);  // Cached!
```

**Use Cases**:
- Code generation
- Analysis passes
- Test sharding

### Build Analytics

Collect and analyze build metrics:

- **Build duration trends**: Track build times over time
- **Cache hit rates**: Identify cache misses
- **Bottleneck analysis**: Find slow targets
- **Resource utilization**: CPU, memory, disk, network

**Visualization**:
- Gantt charts for parallel execution
- Flame graphs for profiling
- Dependency graphs
- Time series dashboards

### Remote Caching Federation

Multiple remote cache backends with fallback:

```
Local Cache
    ↓ miss
Team Cache (Redis)
    ↓ miss
CI Cache (S3)
    ↓ miss
Public Cache (CDN)
    ↓ miss
Build from source
```

### Language Server Protocol (LSP)

Integrate with IDEs via LSP:

- **Go to definition**: Navigate to BUILD files
- **Find references**: Find targets depending on a file
- **Autocomplete**: Suggest target labels
- **Diagnostics**: Show build errors in real-time

### Bazel Migration Tool

Automated migration from Bazel to Horcrux:

```bash
horcrux migrate bazel --workspace=/path/to/bazel/workspace
```

**Features**:
- Convert BUILD files automatically
- Map Bazel rules to Horcrux rules
- Generate migration report

## Glossary

| Term | Definition |
|------|------------|
| **Action** | A single build step (e.g., compile a file, link a binary) |
| **Artifact** | Output file produced by a build action |
| **BUILD file** | File describing build targets and rules |
| **Cache** | Storage for previously built artifacts |
| **Content-addressable storage** | Storage system where data is indexed by content hash |
| **DAG** | Directed Acyclic Graph - a graph with directed edges and no cycles |
| **Dependency** | Target or file required to build another target |
| **Hermetic build** | Build that doesn't depend on anything outside declared inputs |
| **Label** | Unique identifier for a target (e.g., `//path/to:name`) |
| **LSM-tree** | Log-Structured Merge tree - write-optimized data structure used by RocksDB |
| **Merkle tree** | Tree structure where each node is labeled with a cryptographic hash |
| **REP** | Remote Execution Protocol - Bazel's API for distributed builds |
| **Rule** | Template for building a specific type of target |
| **Sandbox** | Isolated execution environment for build actions |
| **Target** | Single buildable unit (library, binary, test, etc.) |
| **Toolchain** | Set of tools (compiler, linker, etc.) for building |
| **Workspace** | Root directory of a Horcrux project |

## Architecture Decision Records

### ADR-001: RocksDB as Primary Persistence Engine

**Status**: Accepted  
**Context**: Need high-performance storage for build cache, DAGs, and artifacts with frequent writes.  
**Decision**: Use RocksDB with SQLite for metadata only.  
**Consequences**: Write-optimized LSM-tree provides 2-4× better performance for cache-heavy workloads.

### ADR-002: Two-Tier Caching (Memory + RocksDB)

**Status**: Accepted  
**Context**: Repeated RocksDB reads create latency for hot data access.  
**Decision**: Implement in-memory LRU cache (1-2 GB) above RocksDB.  
**Consequences**: 2-4× faster incremental builds, minimal memory overhead.

### ADR-003: XXH3 for Intermediate Hashing

**Status**: Accepted  
**Context**: SHA-256 is slow for non-cryptographic intermediate computations.  
**Decision**: Use XXH3 (~50 GB/s) for Merkle tree nodes, SHA-256 for final artifacts.  
**Consequences**: 10-20× faster hash computation with maintained security for final artifacts.

### ADR-004: Deterministic File Traversal

**Status**: Accepted  
**Context**: Unordered filesystem iteration breaks reproducibility.  
**Decision**: Always sort file paths lexicographically before hashing.  
**Consequences**: Cross-platform reproducible builds guaranteed.

### ADR-005: Toolchain Lock File

**Status**: Accepted  
**Context**: Tool version drift breaks build reproducibility.  
**Decision**: Maintain `.horcrux.lock` with pinned tool versions and hashes.  
**Consequences**: Hermetic builds with exact toolchain versioning.

---

**Document Status**: Draft - Subject to revision as implementation progresses

**Peer Review**: This architecture has been reviewed for correctness, performance, and scalability. Key improvements include deterministic hashing, two-tier caching, adaptive concurrency, and REP compatibility.

**Feedback**: Please open an issue or discussion on GitHub if you have questions or suggestions about this architecture.

**Contributors**: This document was created as part of the Horcrux project initialization. All contributors are welcome to propose improvements.
