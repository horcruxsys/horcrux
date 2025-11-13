# GitHub Copilot Instructions for Horcrux

This document provides guidelines for AI-assisted development in the Horcrux project. Following these instructions ensures that all contributions align with Horcrux's vision, architecture, and quality standards.

## 🚀 Vision & Philosophy

Horcrux is a **modern, high-performance, memory-safe, and developer-friendly build system** written in **C++23**, designed to build **any project in any language**. It is built on three foundational principles:

1. **Correctness First** — Builds must always produce consistent, verifiable artifacts.
2. **Speed by Design** — Every computation, cache, and dependency is optimized for minimal latency. Target: 10x faster than Bazel, Buck, or CMake.
3. **Developer Empathy** — Configuration should be intuitive, debuggable, and transparent.

### Core Values

- **Deterministic and Reproducible**: All builds must be hermetic and reproducible across environments
- **Memory Safety**: Zero undefined behavior, use RAII and modern C++ features
- **Performance**: Cache-friendly data structures, lock-free algorithms, parallel execution
- **Modularity**: Hexagonal architecture with clear boundaries between components
- **Testability**: All code must be unit-testable and well-documented
- **Open Source Excellence**: Follow best practices from [opensource.guide](https://opensource.guide/)

## 📖 Code Style Guide

### C++23 Conventions

#### Naming Conventions

- **Variables and functions**: `snake_case`
  ```cpp
  int build_count = 0;
  void execute_build() { }
  ```

- **Types and classes**: `PascalCase`
  ```cpp
  class BuildEngine { };
  struct DependencyGraph { };
  using ErrorCode = int;
  ```

- **Constants**: `SCREAMING_SNAKE_CASE` (or `kPascalCase` for typed constants)
  ```cpp
  constexpr int MAX_PARALLEL_JOBS = 128;
  constexpr auto kDefaultTimeout = std::chrono::seconds(30);
  ```

- **Macros**: `SCREAMING_SNAKE_CASE` (but prefer constexpr/consteval over macros)
  ```cpp
  #define HORCRUX_VERSION_MAJOR 1  // Only when necessary
  ```

#### Modern C++23 Features

- **Use `std::expected` for error handling** (not exceptions in performance-critical paths)
  ```cpp
  auto build(const Config& config) -> std::expected<BuildResult, Error>;
  ```

- **Use `constexpr` and `consteval` where applicable**
  ```cpp
  constexpr auto compute_hash(std::string_view data) -> uint64_t;
  ```

- **Leverage concepts for template constraints**
  ```cpp
  template<typename T>
  concept Buildable = requires(T t) {
      { t.build() } -> std::same_as<BuildResult>;
  };
  ```

- **Use structured bindings and ranges**
  ```cpp
  for (auto&& [key, value] : dependency_map) { }
  auto filtered = targets | std::views::filter(is_buildable);
  ```

#### RAII and Resource Management

- **All resources must use RAII** - no manual new/delete
  ```cpp
  std::unique_ptr<CacheManager> cache_;
  std::shared_ptr<Logger> logger_;
  std::vector<Target> targets_;  // Use containers over raw arrays
  ```

- **Prefer smart pointers** over raw pointers for ownership
  ```cpp
  // Good
  std::unique_ptr<BuildEngine> create_engine();
  
  // Avoid (unless non-owning reference)
  BuildEngine* get_engine();
  ```

- **Use `std::unique_ptr` for exclusive ownership**
- **Use `std::shared_ptr` only when shared ownership is necessary**
- **Use raw pointers or references for non-owning access**

#### Const-Correctness

- **Mark everything const that can be const**
  ```cpp
  class BuildEngine {
  public:
      auto get_status() const -> BuildStatus;  // const member function
      
  private:
      const Config config_;  // immutable after construction
      mutable std::mutex mutex_;  // mutable for thread-safety
  };
  ```

- **Use `const` references for parameters**
  ```cpp
  void process_targets(const std::vector<Target>& targets);
  ```

- **Prefer `std::string_view` for read-only string parameters**
  ```cpp
  auto find_target(std::string_view name) const -> std::optional<Target>;
  ```

#### Immutability and Functional Programming

- **Prefer immutable data structures** where possible
- **Use `const` by default**, remove only when mutation is necessary
- **Avoid side effects** in functions when possible
- **Return new values rather than mutating parameters**
  ```cpp
  // Good - pure function
  auto add_target(DependencyGraph graph, Target target) -> DependencyGraph;
  
  // Avoid - mutation with side effects
  void add_target(DependencyGraph& graph, Target target);
  ```

#### Error Handling

- **Use `std::expected` for recoverable errors**
  ```cpp
  auto parse_config(const std::filesystem::path& path) 
      -> std::expected<Config, ParseError>;
  ```

- **Use exceptions only for exceptional, unrecoverable errors**
- **Always document error conditions**
- **Provide meaningful error messages**

### Safe Concurrency

- **All shared state must be thread-safe**
- **Document thread-safety guarantees** in comments
- **Prefer lock-free algorithms** where possible
  ```cpp
  std::atomic<int> build_count_{0};
  ```

- **Use RAII for lock management**
  ```cpp
  std::scoped_lock lock(mutex_);
  ```

- **Avoid data races** - use proper synchronization
- **Consider using `std::jthread`** for automatic joining

### Memory Safety

- **Zero undefined behavior** - all code must be well-defined
- **No use-after-free, double-free, or memory leaks**
- **Use sanitizers during development**
  ```bash
  cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
  ```

- **Validate all array/pointer accesses**
- **Use `std::span` for safe array views**
- **Check all preconditions and invariants**

### Code Formatting

- **Indent with 4 spaces** (no tabs)
- **Max line length**: 100 characters
- **Braces**: K&R style (opening brace on same line)
  ```cpp
  if (condition) {
      do_something();
  } else {
      do_something_else();
  }
  ```

- **One declaration per line**
  ```cpp
  // Good
  int x = 0;
  int y = 0;
  
  // Avoid
  int x = 0, y = 0;
  ```

### Comments and Documentation

- **Document all public APIs** with clear comments
- **Explain WHY, not just WHAT** in complex code
- **Use Doxygen-style comments** for public interfaces
  ```cpp
  /// Executes the build graph with parallel task execution.
  ///
  /// @param graph The dependency graph to execute
  /// @param max_jobs Maximum number of parallel jobs (0 = auto-detect)
  /// @return Build result or error
  /// @throws Never throws exceptions
  auto execute(const DependencyGraph& graph, int max_jobs = 0) 
      -> std::expected<BuildResult, Error>;
  ```

- **Include usage examples** for non-trivial APIs
- **Keep comments up-to-date** with code changes

### Prefer Composition Over Inheritance

- **Use composition and interfaces** over deep inheritance hierarchies
- **Apply SOLID principles**
- **Design for testability** - use dependency injection
  ```cpp
  // Good - composition with interface
  class BuildEngine {
  private:
      std::unique_ptr<ICacheManager> cache_;
      std::unique_ptr<IScheduler> scheduler_;
  };
  
  // Avoid - deep inheritance
  class BuildEngine : public Engine, public Observable, public Serializable { };
  ```

## 🏗️ Architecture Principles

### Hexagonal Architecture (Ports & Adapters)

Horcrux follows a **hexagonal architecture** pattern:

```
+------------------------------------------------------+
| CLI / API Layer (Adapters)                           |
| User commands, config parsing, logging               |
+------------------------------------------------------+
| Application Layer (Use Cases)                        |
| Build orchestration, command handlers                |
+------------------------------------------------------+
| Domain Layer (Core Business Logic)                   |
| Rule engine, dependency graph, build scheduler       |
| — Pure C++, no external dependencies —               |
+------------------------------------------------------+
| Infrastructure Layer (Adapters)                      |
| Filesystem, compilers, network, caching, sandboxing  |
+------------------------------------------------------+
```

#### Key Principles

1. **Domain Layer is Pure** - No I/O, no external dependencies, fully testable
2. **Dependencies Point Inward** - Outer layers depend on inner layers, never the reverse
3. **Use Interfaces for Adapters** - Define ports as abstract interfaces
4. **Dependency Injection** - Inject concrete implementations at construction/startup

### Modularity

- **Each module has a single responsibility**
- **Clear interfaces between modules**
- **Minimize coupling, maximize cohesion**
- **Design for testability** - mock/stub external dependencies

### Immutability

- **Prefer immutable data structures** for core domain models
- **Use persistent data structures** for efficient immutable updates
- **Pass by const reference** for read-only access
- **Return new values** rather than mutating state

### Isolation of Core Logic

- **Pure business logic** should not depend on I/O or external systems
- **Use dependency injection** for external dependencies
- **Write unit tests without mocks** for pure logic

## 🌍 Open Source Compliance

### Follow opensource.guide Recommendations

Horcrux follows best practices from [opensource.guide](https://opensource.guide/):

- **Clear License**: MIT License in LICENSE file
- **Code of Conduct**: Contributor Covenant (CODE_OF_CONDUCT.md)
- **Contributing Guidelines**: Detailed in CONTRIBUTING.md
- **README**: Comprehensive documentation with examples
- **Issue Templates**: Structured templates for consistency
- **Pull Request Templates**: Clear expectations for contributions

### License Clarity

- **MIT License** applies to all project code
- **Include copyright notices** in all source files when adding new files
- **Document third-party dependencies** and their licenses
- **Avoid GPL and copyleft licenses** in dependencies (unless optional)

### Contributor Covenant

- **Respectful and inclusive community**
- **Zero tolerance for harassment**
- **Report violations to**: conduct@horcruxsys.org
- **See CODE_OF_CONDUCT.md** for full details

### Documentation Requirements

- **Every public API must be documented**
- **Include usage examples** for complex features
- **Keep documentation in sync** with code
- **Write guides for common tasks**
- **Maintain changelog** for releases

## 🔄 Pull Request & Review Standards

### Commit Guidelines

Follow **conventional commit format**:

```
<type>(<scope>): <subject>

<body>

<footer>
```

#### Commit Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style (formatting, no logic changes)
- `refactor`: Code refactoring (no behavior change)
- `perf`: Performance improvements
- `test`: Adding or updating tests
- `chore`: Build, tooling, dependencies

#### Commit Best Practices

- **Use imperative mood** ("add" not "added")
- **Capitalize first letter** of subject
- **No period at end** of subject
- **Limit subject to 50 characters**
- **Wrap body at 72 characters**
- **Explain WHAT and WHY**, not how
- **Reference issues**: `Closes #123`, `Fixes #456`

### Self-Contained Commits

- **Each commit should be atomic** - a single logical change
- **Each commit should build and pass tests**
- **Include rationale** in commit message body
- **Squash work-in-progress commits** before merging

### Code Review Requirements

Before merging, all PRs must:

1. **Pass all automated tests** (unit, integration)
2. **Pass linting and style checks**
3. **Include tests** for new features/bug fixes
4. **Update documentation** if API changes
5. **Have at least one approval** from a maintainer
6. **Be rebased on latest main** (no merge commits in feature branches)

### Performance-Critical PRs

For PRs that affect performance:

- **Include benchmarks** showing before/after results
- **Profile the changes** to identify bottlenecks
- **Document performance characteristics** (time/space complexity)
- **Compare against baseline** (Bazel, Buck, CMake)
- **Run benchmarks multiple times** for statistical significance

Example benchmark format:

```markdown
## Benchmark Results

| Operation | Before | After | Improvement |
|-----------|--------|-------|-------------|
| Build 1000 targets | 45s | 4.2s | 10.7x faster |
| Cache lookup | 120ms | 8ms | 15x faster |

Environment: Ubuntu 22.04, AMD Ryzen 9 5950X, 64GB RAM, NVMe SSD
```

### Review Checklist

- [ ] Code follows style guidelines
- [ ] All tests pass
- [ ] New tests added for new features
- [ ] Documentation updated
- [ ] No undefined behavior (run with sanitizers)
- [ ] Thread-safety verified
- [ ] Performance acceptable (benchmarks if needed)
- [ ] Commit messages follow guidelines
- [ ] No unnecessary dependencies added

## 🧪 Testing Standards

### Test Requirements

- **All new features** must include tests
- **Bug fixes** must include regression tests
- **Tests must be fast** - use mocks for I/O
- **Tests must be deterministic** - no flaky tests
- **Tests must be isolated** - no shared state

### Test Structure

Use **Arrange-Act-Assert** pattern:

```cpp
TEST(BuildEngineTest, ExecutesSimpleGraph) {
    // Arrange - setup test data
    auto config = Config::default_config();
    auto engine = BuildEngine::create(config).value();
    auto graph = create_simple_test_graph();

    // Act - execute the code under test
    auto result = engine.execute(graph);

    // Assert - verify expectations
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->status, BuildStatus::Success);
}
```

### Test Coverage

- Aim for **>80% code coverage** for new code
- **100% coverage for critical paths** (dependency resolution, caching)
- Focus on **edge cases and error paths**

## 🚨 Security Considerations

- **Never commit secrets** to the repository
- **Validate all external inputs**
- **Use secure defaults** (e.g., HTTPS, not HTTP)
- **Avoid command injection** - sanitize shell commands
- **Use cryptographically secure RNG** for security-sensitive operations

## 🎯 Summary

When contributing to Horcrux:

1. **Follow C++23 modern practices** - RAII, const-correctness, std::expected
2. **Write safe, correct code** - zero undefined behavior, thread-safe
3. **Design for performance** - cache-friendly, lock-free, parallel
4. **Follow hexagonal architecture** - pure domain logic, dependency injection
5. **Write comprehensive tests** - unit + integration, fast and deterministic
6. **Document everything** - APIs, rationale, examples
7. **Use conventional commits** - clear history
8. **Include benchmarks** for performance-critical changes
9. **Be a good open source citizen** - respectful, collaborative, thorough

Together, we're building the world's fastest, most correct, and most developer-friendly build system. Thank you for contributing to Horcrux! 🚀
