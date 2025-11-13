# Horcrux Coding Standards and Principles

**Version:** 1.0  
**Last Updated:** 2025-11-13  
**Status:** Active

## Table of Contents

- [Overview](#overview)
- [Core Principles](#core-principles)
- [C++23 Conventions](#c23-conventions)
  - [Language Features](#language-features)
  - [Resource Management](#resource-management)
  - [Const-Correctness](#const-correctness)
  - [Error Handling](#error-handling)
  - [Functional Purity](#functional-purity)
- [Naming Conventions](#naming-conventions)
- [Code Organization](#code-organization)
- [Memory Safety](#memory-safety)
- [Concurrency and Threading](#concurrency-and-threading)
- [Coroutines](#coroutines)
- [Global State Policy](#global-state-policy)
- [Testing Requirements](#testing-requirements)
- [Performance Guidelines](#performance-guidelines)
- [Documentation Standards](#documentation-standards)
- [Code Review Checklist](#code-review-checklist)

## Overview

This document defines the coding standards and principles for the Horcrux build system. All code contributions must adhere to these standards to ensure correctness, safety, performance, and maintainability.

Horcrux is built on three foundational principles:

1. **Correctness First** — Builds must always produce consistent, verifiable artifacts
2. **Speed by Design** — Every computation, cache, and dependency is optimized for minimal latency
3. **Developer Empathy** — Configuration should be intuitive, debuggable, and transparent

These principles guide every architectural and implementation decision in the project.

## Core Principles

### 1. Zero Undefined Behavior

**Policy:** All code must be well-defined according to the C++ standard. No undefined behavior is acceptable.

**Implementation:**
- Use sanitizers during development (AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer)
- Validate all array and pointer accesses
- Check preconditions and invariants
- Use safe abstractions (`std::span`, `std::optional`, `std::expected`)
- Avoid signed integer overflow
- Initialize all variables at declaration

**Example:**
```cpp
// Good - bounds checked
auto get_element(const std::vector<int>& vec, size_t index) 
    -> std::optional<int> {
    if (index >= vec.size()) {
        return std::nullopt;
    }
    return vec[index];
}

// Bad - undefined behavior if index out of bounds
int get_element_unsafe(const std::vector<int>& vec, size_t index) {
    return vec[index];  // No bounds check!
}
```

### 2. Memory Safety

**Policy:** Zero memory leaks, use-after-free, double-free, or buffer overflows.

**Implementation:**
- Use RAII for all resource management
- Prefer smart pointers over raw pointers for ownership
- Use `std::unique_ptr` for exclusive ownership
- Use `std::shared_ptr` only when shared ownership is necessary
- Use raw pointers or references only for non-owning access
- Avoid manual `new`/`delete`
- Use standard containers (`std::vector`, `std::string`) instead of manual memory management

### 3. Thread Safety

**Policy:** All shared state must be thread-safe. Data races are prohibited.

**Implementation:**
- Document thread-safety guarantees for all public APIs
- Use proper synchronization primitives (`std::mutex`, `std::atomic`)
- Prefer lock-free algorithms where possible
- Use RAII for lock management (`std::scoped_lock`, `std::unique_lock`)
- Avoid deadlocks through consistent lock ordering
- Consider using `std::jthread` for automatic thread joining

### 4. Correctness Over Performance

**Policy:** Correct code first, then optimize if needed with profiling data.

**Implementation:**
- Write correct, readable code first
- Profile before optimizing
- Document performance characteristics (time/space complexity)
- Preserve correctness when optimizing
- Use benchmarks to validate optimizations

### 5. Modularity and Testability

**Policy:** Code must be modular, loosely coupled, and easily testable.

**Implementation:**
- Follow SOLID principles
- Use dependency injection
- Prefer composition over inheritance
- Design pure functions where possible
- Keep functions small and focused
- Make code testable without mocks when possible

## C++23 Conventions

### Language Features

#### Modern Features to Use

**1. `std::expected` for Error Handling**

Use `std::expected` for recoverable errors instead of exceptions in performance-critical paths:

```cpp
auto parse_config(const std::filesystem::path& path) 
    -> std::expected<Config, ParseError> {
    
    if (!std::filesystem::exists(path)) {
        return std::unexpected(ParseError::FileNotFound);
    }
    
    // Parse and return config
    return Config{/* ... */};
}

// Usage
auto config = parse_config("config.json");
if (!config) {
    std::cerr << "Error: " << config.error().message() << '\n';
    return 1;
}
// Use config.value()
```

**2. `constexpr` and `consteval`**

Use `constexpr` for compile-time evaluation and `consteval` for functions that must be evaluated at compile time:

```cpp
constexpr auto compute_hash(std::string_view data) -> uint64_t {
    // Compile-time hash computation when possible
    uint64_t hash = 0;
    for (char c : data) {
        hash = hash * 31 + c;
    }
    return hash;
}

consteval auto get_version_string() -> std::string_view {
    return "1.0.0";  // Must be compile-time constant
}
```

**3. Concepts for Template Constraints**

Use concepts to express template requirements clearly:

```cpp
template<typename T>
concept Buildable = requires(T t) {
    { t.build() } -> std::same_as<BuildResult>;
    { t.get_dependencies() } -> std::convertible_to<std::vector<Label>>;
};

template<Buildable T>
auto execute_build(T& target) -> BuildResult {
    return target.build();
}
```

**4. Ranges and Views**

Use ranges and views for expressive, composable algorithms:

```cpp
auto get_changed_targets(const std::vector<Target>& targets) {
    return targets 
        | std::views::filter([](const Target& t) { return t.is_dirty(); })
        | std::views::transform([](const Target& t) { return t.label(); })
        | std::ranges::to<std::vector>();
}
```

**5. Structured Bindings**

Use structured bindings for multiple return values and tuple-like types:

```cpp
for (auto&& [key, value] : dependency_map) {
    process(key, value);
}

auto [result, error] = parse_build_file(path);
if (error) {
    handle_error(error);
}
```

**6. `std::optional` for Optional Values**

Use `std::optional` for values that may or may not exist:

```cpp
auto find_target(std::string_view name) const 
    -> std::optional<Target> {
    
    auto it = targets_.find(name);
    if (it != targets_.end()) {
        return it->second;
    }
    return std::nullopt;
}

// Usage
if (auto target = find_target("//app:main")) {
    build(*target);
}
```

### Resource Management

#### RAII (Resource Acquisition Is Initialization)

**Policy:** All resources must be managed using RAII. No manual resource management.

**Examples:**

```cpp
// Good - RAII with smart pointers
class BuildEngine {
private:
    std::unique_ptr<CacheManager> cache_;
    std::shared_ptr<Logger> logger_;
    std::vector<Target> targets_;
    std::fstream config_file_;  // Automatically closed
};

// Good - RAII with custom resource
class FileDescriptor {
public:
    explicit FileDescriptor(const char* path) 
        : fd_(open(path, O_RDONLY)) {
        if (fd_ < 0) {
            throw std::runtime_error("Failed to open file");
        }
    }
    
    ~FileDescriptor() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }
    
    // Delete copy, allow move
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    FileDescriptor(FileDescriptor&& other) noexcept 
        : fd_(std::exchange(other.fd_, -1)) {}
    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) close(fd_);
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }
    
    int get() const { return fd_; }
    
private:
    int fd_;
};

// Bad - manual memory management
class BadEngine {
private:
    CacheManager* cache_;  // Who owns this?
    
    ~BadEngine() {
        delete cache_;  // Manual cleanup - error-prone
    }
};
```

#### Smart Pointer Guidelines

**1. `std::unique_ptr` - Exclusive Ownership**

Use `std::unique_ptr` when a single owner manages the resource:

```cpp
auto create_cache() -> std::unique_ptr<CacheManager> {
    return std::make_unique<CacheManager>();
}

class BuildEngine {
private:
    std::unique_ptr<CacheManager> cache_;  // Exclusive ownership
};
```

**2. `std::shared_ptr` - Shared Ownership**

Use `std::shared_ptr` only when multiple owners need to share ownership:

```cpp
class Logger {
    // Logger shared across components
};

auto create_logger() -> std::shared_ptr<Logger> {
    return std::make_shared<Logger>();
}

class BuildEngine {
private:
    std::shared_ptr<Logger> logger_;  // Shared with other components
};
```

**3. Raw Pointers and References - Non-Owning**

Use raw pointers or references for non-owning access:

```cpp
class Scheduler {
public:
    // Non-owning reference to cache
    auto set_cache(CacheManager* cache) -> void {
        cache_ = cache;  // Doesn't own, just uses
    }
    
    // Or use reference if never null
    auto execute(const BuildGraph& graph) -> BuildResult;
    
private:
    CacheManager* cache_{nullptr};  // Non-owning
};
```

### Const-Correctness

**Policy:** Mark everything `const` that can be `const`. Use `const` by default, remove only when mutation is necessary.

#### Const Member Functions

```cpp
class BuildGraph {
public:
    // Const member functions don't modify state
    auto get_target(const Label& label) const -> std::optional<Target>;
    auto get_dependencies(const Label& label) const -> std::vector<Label>;
    auto is_empty() const -> bool;
    
    // Non-const modifies state
    auto add_target(Target target) -> void;
    
private:
    std::unordered_map<Label, Target> targets_;
    mutable std::mutex mutex_;  // Mutable for thread-safety in const methods
};
```

#### Const Parameters

```cpp
// Good - const reference for read-only parameters
auto process_targets(const std::vector<Target>& targets) -> void;

// Good - pass by value for small types
auto compute_hash(Label label) -> uint64_t;

// Good - std::string_view for read-only strings
auto find_target(std::string_view name) const -> std::optional<Target>;

// Bad - non-const reference suggests modification
auto process_targets(std::vector<Target>& targets) -> void;  // Unless actually modifying
```

#### Const Data Members

```cpp
class BuildEngine {
public:
    explicit BuildEngine(Config config) 
        : config_(std::move(config)) {}
    
private:
    const Config config_;  // Immutable after construction
    mutable std::mutex mutex_;  // Mutable for synchronization
};
```

### Error Handling

**Policy:** Use `std::expected` for recoverable errors, exceptions only for exceptional, unrecoverable errors.

#### When to Use `std::expected`

Use `std::expected` for:
- Parse errors
- File I/O errors
- Network errors
- Validation errors
- Any recoverable error condition

```cpp
enum class ParseError {
    FileNotFound,
    SyntaxError,
    InvalidTarget,
    CircularDependency
};

auto parse_build_file(const Path& path) 
    -> std::expected<BuildGraph, ParseError> {
    
    if (!std::filesystem::exists(path)) {
        return std::unexpected(ParseError::FileNotFound);
    }
    
    auto content = read_file(path);
    if (has_syntax_error(content)) {
        return std::unexpected(ParseError::SyntaxError);
    }
    
    return BuildGraph{/* ... */};
}

// Usage with monadic operations
auto result = parse_build_file(path)
    .and_then([](BuildGraph graph) { return validate_graph(graph); })
    .and_then([](BuildGraph graph) { return optimize_graph(graph); })
    .transform([](BuildGraph graph) { return execute_build(graph); });

if (!result) {
    handle_error(result.error());
}
```

#### When to Use Exceptions

Use exceptions only for:
- Programming errors (assertion failures, contract violations)
- Unrecoverable errors (out of memory, system errors)
- Constructor failures
- Library/framework errors that propagate

```cpp
class BuildEngine {
public:
    explicit BuildEngine(Config config) {
        if (!config.is_valid()) {
            throw std::invalid_argument("Invalid configuration");
        }
        config_ = std::move(config);
    }
};
```

#### Error Documentation

Always document error conditions:

```cpp
/// Parses a BUILD file and constructs a dependency graph.
///
/// @param path Path to the BUILD file
/// @return Expected BuildGraph or ParseError
/// @retval ParseError::FileNotFound if file doesn't exist
/// @retval ParseError::SyntaxError if file has syntax errors
/// @retval ParseError::CircularDependency if graph has cycles
auto parse_build_file(const Path& path) 
    -> std::expected<BuildGraph, ParseError>;
```

### Functional Purity

**Policy:** Prefer pure functions (no side effects) where possible. Return new values rather than mutating parameters.

#### Pure Functions

```cpp
// Good - pure function, no side effects
auto add_target(BuildGraph graph, Target target) -> BuildGraph {
    auto new_graph = graph;  // Copy
    new_graph.targets_.insert({target.label(), target});
    return new_graph;
}

// Good - transformation returns new value
auto filter_dirty_targets(const std::vector<Target>& targets) 
    -> std::vector<Target> {
    return targets 
        | std::views::filter([](const Target& t) { return t.is_dirty(); })
        | std::ranges::to<std::vector>();
}

// Avoid - mutation with side effects (use only when necessary for performance)
auto add_target_inplace(BuildGraph& graph, Target target) -> void {
    graph.targets_.insert({target.label(), target});
}
```

#### Immutable Data Structures

Prefer immutable data structures where possible:

```cpp
class BuildGraph {
public:
    // Immutable interface - returns new graph
    auto with_target(Target target) const -> BuildGraph;
    auto without_target(const Label& label) const -> BuildGraph;
    
    // Query operations (const)
    auto get_target(const Label& label) const -> std::optional<Target>;
    auto get_dependencies(const Label& label) const -> std::vector<Label>;
    
private:
    std::unordered_map<Label, Target> targets_;
};
```

## Naming Conventions

### Variables and Functions: `snake_case`

```cpp
int build_count = 0;
std::string target_name;

auto execute_build() -> void;
auto get_target_label() -> Label;
```

### Types and Classes: `PascalCase`

```cpp
class BuildEngine {};
struct DependencyGraph {};
enum class BuildStatus {};
using ErrorCode = int;

template<typename T>
concept Buildable = /* ... */;
```

### Constants: `SCREAMING_SNAKE_CASE` or `kPascalCase`

```cpp
// Global constants
constexpr int MAX_PARALLEL_JOBS = 128;
constexpr size_t DEFAULT_CACHE_SIZE = 1024 * 1024 * 1024;

// Typed constants (preferred)
constexpr auto kDefaultTimeout = std::chrono::seconds(30);
constexpr auto kMaxRetries = 3;
```

### Macros: `SCREAMING_SNAKE_CASE` (Avoid When Possible)

```cpp
#define HORCRUX_VERSION_MAJOR 1
#define HORCRUX_VERSION_MINOR 0

// Prefer constexpr over macros
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
```

### Private Members: Trailing Underscore

```cpp
class BuildEngine {
private:
    Config config_;
    std::unique_ptr<CacheManager> cache_;
    std::mutex mutex_;
};
```

### File Names: `snake_case`

```
build_engine.h
build_engine.cpp
dependency_graph.h
cache_manager.h
```

## Code Organization

### File Structure

```cpp
// build_engine.h

#pragma once

#include <expected>
#include <memory>
#include <vector>

namespace horcrux {

/// BuildEngine orchestrates the build process.
///
/// The BuildEngine is responsible for coordinating the build graph,
/// execution scheduling, and cache management.
class BuildEngine {
public:
    /// Creates a new BuildEngine with the given configuration.
    ///
    /// @param config Build configuration
    /// @return Expected BuildEngine or error
    static auto create(Config config) 
        -> std::expected<BuildEngine, Error>;
    
    /// Executes the build for the given targets.
    ///
    /// @param targets Target labels to build
    /// @return Expected BuildResult or error
    auto execute(const std::vector<Label>& targets) 
        -> std::expected<BuildResult, Error>;
    
private:
    explicit BuildEngine(Config config);
    
    Config config_;
    std::unique_ptr<CacheManager> cache_;
    std::unique_ptr<Scheduler> scheduler_;
};

}  // namespace horcrux
```

### Header Guards

Use `#pragma once` for header guards:

```cpp
#pragma once

// Header content
```

### Include Order

1. Related header (for .cpp files)
2. C standard library headers
3. C++ standard library headers
4. Third-party library headers
5. Project headers

```cpp
// build_engine.cpp

#include "build_engine.h"  // Related header first

#include <cstdint>         // C standard library
#include <cstring>

#include <algorithm>       // C++ standard library
#include <memory>
#include <vector>

#include <absl/container/flat_hash_map.h>  // Third-party

#include "cache_manager.h"  // Project headers
#include "scheduler.h"
```

### Namespace Usage

```cpp
namespace horcrux {

// All Horcrux code in namespace

namespace detail {
// Implementation details
}  // namespace detail

}  // namespace horcrux
```

Do not use `using namespace` in headers. In implementation files, it's acceptable for standard library:

```cpp
// Acceptable in .cpp files
using namespace std::string_literals;
using namespace std::chrono_literals;
```

## Memory Safety

### Guidelines

1. **No manual memory management** - use RAII and smart pointers
2. **Bounds checking** - validate all array/vector accesses
3. **Use `std::span`** for array views
4. **Use `std::string_view`** for string views
5. **Initialize all variables** at declaration
6. **Use `std::optional`** for optional values instead of null pointers

### Safe Array Access

```cpp
// Good - bounds checked
auto get_element(const std::vector<int>& vec, size_t index) 
    -> std::optional<int> {
    if (index >= vec.size()) {
        return std::nullopt;
    }
    return vec[index];
}

// Good - use .at() for automatic bounds checking
try {
    int value = vec.at(index);
} catch (const std::out_of_range&) {
    // Handle error
}

// Good - use std::span for safe array views
auto process_data(std::span<const int> data) -> void {
    for (int value : data) {
        process(value);
    }
}
```

### Sanitizer Usage

Always develop with sanitizers enabled:

```bash
# AddressSanitizer - detects memory errors
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address"

# UndefinedBehaviorSanitizer - detects undefined behavior
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=undefined"

# ThreadSanitizer - detects data races
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=thread"

# Combined (except thread, which conflicts with address)
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
```

## Concurrency and Threading

### Threading Policy

**Policy:** All shared state must be explicitly synchronized. Data races are prohibited.

### Thread-Safety Guarantees

Document thread-safety for all classes:

```cpp
/// BuildGraph is immutable and thread-safe for concurrent reads.
/// Thread-safety: Safe for concurrent reads, no writes after construction.
class BuildGraph {
    // Immutable, thread-safe
};

/// CacheManager is thread-safe for all operations.
/// Thread-safety: All methods are internally synchronized.
class CacheManager {
public:
    auto lookup(const Hash& hash) -> std::optional<Artifact>;  // Thread-safe
    auto store(const Hash& hash, Artifact artifact) -> void;   // Thread-safe
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<Hash, Artifact> cache_;
};

/// BuildEngine is NOT thread-safe.
/// Thread-safety: Not thread-safe. Caller must synchronize.
class BuildEngine {
    // Single-threaded use only
};
```

### Synchronization Primitives

#### Mutex

Use `std::mutex` for protecting shared state:

```cpp
class Counter {
public:
    auto increment() -> void {
        std::scoped_lock lock(mutex_);
        ++count_;
    }
    
    auto get() const -> int {
        std::scoped_lock lock(mutex_);
        return count_;
    }
    
private:
    mutable std::mutex mutex_;
    int count_{0};
};
```

#### Atomic Variables

Use `std::atomic` for simple thread-safe counters and flags:

```cpp
class BuildEngine {
private:
    std::atomic<int> build_count_{0};
    std::atomic<bool> is_running_{false};
};

// Usage
build_count_.fetch_add(1, std::memory_order_relaxed);
if (is_running_.load(std::memory_order_acquire)) {
    // ...
}
```

#### Lock-Free Algorithms

Prefer lock-free algorithms when possible:

```cpp
// Lock-free queue for task distribution
template<typename T>
class LockFreeQueue {
public:
    auto push(T value) -> void {
        auto new_node = new Node{std::move(value), nullptr};
        Node* old_head = head_.load(std::memory_order_relaxed);
        do {
            new_node->next = old_head;
        } while (!head_.compare_exchange_weak(old_head, new_node,
                                              std::memory_order_release,
                                              std::memory_order_relaxed));
    }
    
private:
    struct Node {
        T data;
        std::atomic<Node*> next;
    };
    std::atomic<Node*> head_{nullptr};
};
```

### Thread Management

Use `std::jthread` for automatic joining:

```cpp
class BuildEngine {
public:
    auto start_background_worker() -> void {
        worker_ = std::jthread([this](std::stop_token token) {
            while (!token.stop_requested()) {
                process_task();
            }
        });
    }
    
private:
    std::jthread worker_;  // Automatically joined in destructor
};
```

### Deadlock Prevention

**Guidelines:**
1. Always acquire locks in the same order
2. Use `std::scoped_lock` for multiple locks (prevents deadlock)
3. Minimize lock duration
4. Avoid calling callbacks while holding locks

```cpp
// Good - scoped_lock prevents deadlock
auto transfer(Account& from, Account& to, double amount) -> void {
    std::scoped_lock lock(from.mutex_, to.mutex_);  // Acquires in order
    from.balance_ -= amount;
    to.balance_ += amount;
}

// Bad - potential deadlock
auto transfer_bad(Account& from, Account& to, double amount) -> void {
    std::unique_lock lock1(from.mutex_);
    std::unique_lock lock2(to.mutex_);  // Can deadlock if other thread locks in reverse order
    from.balance_ -= amount;
    to.balance_ += amount;
}
```

## Coroutines

### Coroutine Policy

**Policy:** Use C++20 coroutines for asynchronous operations. Prefer structured concurrency.

### Coroutine Guidelines

```cpp
// Coroutine for async I/O
auto read_file_async(const Path& path) -> Task<std::string> {
    auto file = co_await open_file(path);
    auto content = co_await file.read_all();
    co_return content;
}

// Usage
auto content = co_await read_file_async("config.json");
```

### Structured Concurrency

```cpp
// Run multiple tasks concurrently
auto build_all_targets(const std::vector<Label>& targets) 
    -> Task<std::vector<BuildResult>> {
    
    std::vector<Task<BuildResult>> tasks;
    for (const auto& target : targets) {
        tasks.push_back(build_target(target));
    }
    
    co_return co_await when_all(tasks);
}
```

### Error Handling in Coroutines

```cpp
auto build_target(const Label& label) 
    -> Task<std::expected<BuildResult, Error>> {
    
    try {
        auto graph = co_await load_graph(label);
        auto result = co_await execute_build(graph);
        co_return result;
    } catch (const std::exception& e) {
        co_return std::unexpected(Error{e.what()});
    }
}
```

## Global State Policy

### No Global Mutable State

**Policy:** Global mutable state is prohibited. All state must be explicitly managed through object lifetimes.

**Rationale:**
- Global mutable state makes testing difficult
- Creates hidden dependencies between components
- Causes race conditions in multi-threaded code
- Makes code harder to reason about

**Allowed:**
- Global constants (`constexpr`, `const`)
- Global immutable configuration (initialized once at startup)
- Thread-local storage for thread-specific state

**Prohibited:**
- Global variables
- Singletons with mutable state
- Static class members with mutable state

```cpp
// Good - immutable global constants
namespace config {
constexpr int MAX_PARALLEL_JOBS = 128;
constexpr auto DEFAULT_CACHE_DIR = "/var/cache/horcrux";
}

// Good - const global configuration (initialized once)
const Config& get_global_config() {
    static const Config config = load_config();
    return config;
}

// Good - thread-local for thread-specific state
thread_local int current_thread_id = 0;

// Bad - global mutable state
static int global_build_count = 0;  // Prohibited!

// Bad - singleton with mutable state
class GlobalCache {
public:
    static GlobalCache& instance() {
        static GlobalCache cache;
        return cache;
    }
    
    auto add(Hash hash, Artifact artifact) -> void {
        cache_[hash] = artifact;  // Mutable global state!
    }
    
private:
    std::unordered_map<Hash, Artifact> cache_;
};
```

### Dependency Injection

Use dependency injection instead of global state:

```cpp
// Good - dependency injection
class BuildEngine {
public:
    explicit BuildEngine(
        std::unique_ptr<CacheManager> cache,
        std::shared_ptr<Logger> logger)
        : cache_(std::move(cache))
        , logger_(std::move(logger)) {}
    
private:
    std::unique_ptr<CacheManager> cache_;
    std::shared_ptr<Logger> logger_;
};

// Usage
auto cache = std::make_unique<CacheManager>();
auto logger = std::make_shared<Logger>();
auto engine = BuildEngine(std::move(cache), logger);
```

## Testing Requirements

### Test Coverage

**Requirements:**
- **All new features** must include unit tests
- **Bug fixes** must include regression tests
- **Aim for >80% code coverage** for new code
- **100% coverage** for critical paths (dependency resolution, caching, scheduling)

### Test Types

#### Unit Tests

Test individual components in isolation:

```cpp
#include <gtest/gtest.h>

TEST(BuildGraphTest, AddTarget) {
    // Arrange
    BuildGraph graph;
    Target target{"//app:main", TargetType::Binary};
    
    // Act
    graph.add_target(target);
    
    // Assert
    EXPECT_TRUE(graph.has_target("//app:main"));
    auto retrieved = graph.get_target("//app:main");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->label(), "//app:main");
}

TEST(BuildGraphTest, DetectsCircularDependency) {
    // Arrange
    BuildGraph graph;
    graph.add_target(Target{"//a", TargetType::Library});
    graph.add_target(Target{"//b", TargetType::Library});
    graph.add_dependency("//a", "//b");
    
    // Act & Assert - should detect cycle
    EXPECT_FALSE(graph.add_dependency("//b", "//a").has_value());
}
```

#### Integration Tests

Test component interactions:

```cpp
TEST(BuildEngineIntegrationTest, BuildSimpleProject) {
    // Arrange
    auto temp_dir = create_temp_workspace();
    write_file(temp_dir / "BUILD", R"(
        cc_binary(
            name = "app",
            srcs = ["main.cpp"],
        )
    )");
    write_file(temp_dir / "main.cpp", "int main() { return 0; }");
    
    auto config = Config::for_workspace(temp_dir);
    auto engine = BuildEngine::create(config).value();
    
    // Act
    auto result = engine.execute({"//app:app"});
    
    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->status, BuildStatus::Success);
    EXPECT_TRUE(std::filesystem::exists(temp_dir / "bazel-bin/app/app"));
}
```

#### Benchmark Tests

Test performance characteristics:

```cpp
#include <benchmark/benchmark.h>

static void BM_CacheLookup(benchmark::State& state) {
    CacheManager cache;
    Hash hash = compute_hash("test_data");
    Artifact artifact{"data", 1024};
    cache.store(hash, artifact);
    
    for (auto _ : state) {
        auto result = cache.lookup(hash);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_CacheLookup);

static void BM_DependencyResolution(benchmark::State& state) {
    auto graph = create_large_test_graph(state.range(0));
    
    for (auto _ : state) {
        auto deps = graph.resolve_dependencies("//root:target");
        benchmark::DoNotOptimize(deps);
    }
}
BENCHMARK(BM_DependencyResolution)->Range(100, 10000);
```

### Test Structure

Use **Arrange-Act-Assert** pattern:

```cpp
TEST(BuildEngineTest, ExecutesBuildSuccessfully) {
    // Arrange - setup test data and dependencies
    auto config = Config::default_config();
    auto engine = BuildEngine::create(config).value();
    auto graph = create_simple_test_graph();
    
    // Act - execute the code under test
    auto result = engine.execute(graph);
    
    // Assert - verify expectations
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->status, BuildStatus::Success);
    EXPECT_GT(result->duration, std::chrono::milliseconds(0));
}
```

### Test Guidelines

1. **Tests must be fast** - use mocks/stubs for I/O operations
2. **Tests must be deterministic** - no flaky tests
3. **Tests must be isolated** - no shared state between tests
4. **One assertion per test** when possible
5. **Use descriptive test names** that explain what is being tested
6. **Clean up resources** in test teardown

```cpp
// Good test name - explains what is tested and expected behavior
TEST(CacheManagerTest, ReturnsNulloptWhenKeyNotFound) {
    // ...
}

// Bad test name - unclear what is being tested
TEST(CacheManagerTest, Test1) {
    // ...
}
```

## Performance Guidelines

### General Principles

1. **Correct first, fast second** - optimize only with profiling data
2. **Measure, don't guess** - use benchmarks to validate optimizations
3. **Document complexity** - include time/space complexity in comments
4. **Cache-friendly data structures** - consider memory layout
5. **Minimize allocations** - reuse objects in hot paths

### Cache-Friendly Code

```cpp
// Good - struct of arrays (cache-friendly)
struct ParticleSystem {
    std::vector<float> positions_x;
    std::vector<float> positions_y;
    std::vector<float> positions_z;
    std::vector<float> velocities_x;
    std::vector<float> velocities_y;
    std::vector<float> velocities_z;
};

// Bad - array of structs (cache-unfriendly for sequential access)
struct Particle {
    float pos_x, pos_y, pos_z;
    float vel_x, vel_y, vel_z;
};
std::vector<Particle> particles;
```

### Avoid Allocations in Hot Paths

```cpp
// Good - preallocate and reuse
class Scheduler {
public:
    auto schedule_tasks(const std::vector<Task>& tasks) -> void {
        ready_queue_.clear();  // Reuse existing allocation
        for (const auto& task : tasks) {
            if (task.is_ready()) {
                ready_queue_.push_back(task);
            }
        }
    }
    
private:
    std::vector<Task> ready_queue_;  // Reused across calls
};

// Bad - allocates on every call
auto schedule_tasks(const std::vector<Task>& tasks) 
    -> std::vector<Task> {
    std::vector<Task> ready_queue;  // New allocation each time
    for (const auto& task : tasks) {
        if (task.is_ready()) {
            ready_queue.push_back(task);
        }
    }
    return ready_queue;
}
```

### Complexity Documentation

```cpp
/// Finds all targets affected by the given file changes.
///
/// @param graph The build dependency graph
/// @param changed_files Files that have been modified
/// @return List of affected target labels
///
/// @complexity O(V + E) where V = number of targets, E = number of dependencies
/// @space O(V) for visited set and result
auto find_affected_targets(
    const BuildGraph& graph,
    const std::vector<Path>& changed_files
) -> std::vector<Label>;
```

## Documentation Standards

### Public API Documentation

All public APIs must be documented with Doxygen-style comments:

```cpp
/// Executes the build graph with parallel task execution.
///
/// This function schedules and executes all tasks in the build graph
/// according to their dependency relationships. Tasks are executed in
/// parallel up to the configured maximum number of jobs.
///
/// @param graph The dependency graph to execute
/// @param max_jobs Maximum number of parallel jobs (0 = auto-detect)
/// @return Build result on success, error on failure
///
/// @pre graph must be a valid DAG with no cycles
/// @post All tasks in graph are either completed or failed
///
/// @throws Never throws exceptions
///
/// @note This function is thread-safe and can be called concurrently
///
/// @example
/// @code
/// auto config = Config::default_config();
/// auto engine = BuildEngine::create(config).value();
/// auto graph = load_build_graph("//...");
/// auto result = engine.execute(graph, 8);
/// if (result) {
///     std::cout << "Build succeeded!\n";
/// }
/// @endcode
auto execute(const DependencyGraph& graph, int max_jobs = 0) 
    -> std::expected<BuildResult, Error>;
```

### Documentation Requirements

1. **Purpose** - What does the function/class do?
2. **Parameters** - Document all parameters with `@param`
3. **Return value** - Document with `@return` or `@retval`
4. **Exceptions** - Document exceptions with `@throws`
5. **Preconditions** - Document with `@pre`
6. **Postconditions** - Document with `@post`
7. **Thread-safety** - Document with `@note`
8. **Complexity** - Document with `@complexity`
9. **Examples** - Include usage examples with `@example` and `@code`

### Code Comments

```cpp
// Explain WHY, not WHAT
// Good - explains rationale
// Use double hashing to reduce collision probability for large hash tables
auto compute_double_hash(uint64_t key) -> size_t;

// Bad - restates the obvious
// Add x and y
auto add(int x, int y) -> int;

// Good - explains non-obvious behavior
// Sort before hashing to ensure deterministic results across different
// filesystem orderings and platforms
std::sort(files.begin(), files.end());
auto hash = compute_merkle_hash(files);
```

## Code Review Checklist

Use this checklist when reviewing code:

### Correctness
- [ ] Code produces correct results for all inputs
- [ ] Edge cases are handled (empty inputs, null values, boundary conditions)
- [ ] Error conditions are properly handled
- [ ] No undefined behavior (run with sanitizers)
- [ ] No data races (run with ThreadSanitizer)
- [ ] No memory leaks (run with AddressSanitizer)

### Safety
- [ ] All resources use RAII
- [ ] Smart pointers used for ownership
- [ ] All array accesses are bounds-checked
- [ ] No manual memory management (`new`/`delete`)
- [ ] Const-correctness enforced
- [ ] Thread-safety guarantees documented

### Design
- [ ] Code follows SOLID principles
- [ ] Functions are small and focused (< 50 lines typically)
- [ ] Classes have single responsibility
- [ ] Interfaces are minimal and cohesive
- [ ] Composition preferred over inheritance
- [ ] Dependency injection used appropriately

### Style
- [ ] Naming conventions followed (snake_case, PascalCase)
- [ ] Code is properly formatted (4 spaces, max 100 chars)
- [ ] No magic numbers (use named constants)
- [ ] No global mutable state
- [ ] Modern C++23 features used appropriately

### Documentation
- [ ] Public APIs documented with Doxygen comments
- [ ] Complex code has explanatory comments
- [ ] Thread-safety guarantees documented
- [ ] Performance characteristics documented
- [ ] Usage examples provided for non-trivial APIs

### Testing
- [ ] Unit tests included for new features
- [ ] Integration tests for component interactions
- [ ] Benchmarks for performance-critical code
- [ ] Tests follow Arrange-Act-Assert pattern
- [ ] Tests are fast, deterministic, and isolated
- [ ] Test coverage >80% for new code

### Performance
- [ ] No obvious performance issues
- [ ] Algorithms have reasonable complexity
- [ ] Hot paths avoid allocations
- [ ] Data structures are cache-friendly
- [ ] Profiling data supports optimizations

### Concurrency
- [ ] Thread-safety guarantees clear and enforced
- [ ] Proper synchronization used
- [ ] No deadlock potential
- [ ] Lock-free algorithms used where appropriate
- [ ] Atomic operations use appropriate memory ordering

### Error Handling
- [ ] `std::expected` used for recoverable errors
- [ ] Error conditions properly documented
- [ ] Exceptions used only for exceptional cases
- [ ] Error messages are clear and actionable

## Validation

This document serves as the authoritative guide for code standards in the Horcrux project. All code contributions will be reviewed against these standards. When in doubt, refer to this document or ask for clarification in code reviews.

---

**Document Maintainers:** Horcrux Core Team  
**Review Cycle:** Quarterly or as needed  
**Feedback:** Open an issue or PR to suggest improvements to this document
