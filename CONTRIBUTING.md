# Contributing to Horcrux

Thank you for your interest in contributing to Horcrux! We're building the next-generation universal build system, and we welcome contributions from developers of all skill levels.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
  - [Reporting Bugs](#reporting-bugs)
  - [Suggesting Enhancements](#suggesting-enhancements)
  - [Your First Code Contribution](#your-first-code-contribution)
  - [Pull Requests](#pull-requests)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Testing Guidelines](#testing-guidelines)
- [Commit Message Guidelines](#commit-message-guidelines)
- [Review Process](#review-process)
- [Community](#community)

## Code of Conduct

This project and everyone participating in it is governed by our [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to [conduct@horcruxsys.org](mailto:conduct@horcruxsys.org).

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check the [issue tracker](https://github.com/horcruxsys/horcrux/issues) to avoid duplicates. When creating a bug report, please include as many details as possible:

**Great bug reports include:**

- **Clear and descriptive title** that identifies the problem
- **Exact steps to reproduce** the issue
- **Expected behavior** vs. **actual behavior**
- **Environment details**: OS, compiler version, CMake version, etc.
- **Code samples** or minimal reproducible examples
- **Error messages** and logs (use code blocks for readability)
- **Screenshots** if applicable

**Use this template:**

```markdown
## Description
[A clear description of the bug]

## Steps to Reproduce
1. Step one
2. Step two
3. ...

## Expected Behavior
[What should happen]

## Actual Behavior
[What actually happens]

## Environment
- OS: [e.g., Ubuntu 22.04, macOS 14, Windows 11]
- Compiler: [e.g., GCC 13.2, Clang 17, MSVC 2022]
- CMake: [e.g., 3.28]
- Horcrux version: [e.g., commit hash or tag]

## Additional Context
[Any other relevant information]
```

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion:

- **Use a clear and descriptive title**
- **Provide a detailed description** of the proposed enhancement
- **Explain why this enhancement would be useful** to most Horcrux users
- **List examples** of how the feature would be used
- **Mention alternative solutions** you've considered

### Your First Code Contribution

Unsure where to begin? Look for issues labeled:

- `good-first-issue` - Good for newcomers
- `help-wanted` - Issues where we need help
- `documentation` - Documentation improvements

Feel free to ask questions in the issue comments or on our discussion forum.

**Contributing to Examples:**

For Android development examples, contribute to the separate [horcrux-android-examples](https://github.com/horcruxsys/horcrux-android-examples) repository:

- Add new Android example applications
- Improve existing examples with better patterns
- Add unit and instrumentation tests
- Improve documentation and migration guides
- Fix build issues and update dependencies
- Update to latest Android/Kotlin/Compose versions

See [docs/android-examples.md](docs/android-examples.md) for the complete specification of the Android examples repository.

### Pull Requests

1. **Fork the repository** and create your branch from `main`:

   ```bash
   git checkout -b feature/my-new-feature
   ```

2. **Make your changes**:
   - Follow our [coding standards](#coding-standards)
   - Add tests for new functionality
   - Update documentation as needed

3. **Test your changes**:

   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   cmake --build . -- -j$(nproc)
   ctest --output-on-failure
   ```

4. **Commit your changes** using clear commit messages (see [guidelines](#commit-message-guidelines))

5. **Push to your fork** and submit a pull request to the `main` branch

6. **Respond to review feedback** - maintainers may request changes

## Development Setup

### Prerequisites

- **C++23 compiler**: GCC 13+, Clang 17+, or MSVC 2022+
- **CMake 3.25+**
- **Ninja or Make**
- **Git**

### Building from Source

```bash
# Clone the repository
git clone https://github.com/horcruxsys/horcrux.git
cd horcrux

# Install git hooks (lefthook)
# This sets up pre-commit and pre-push hooks for code quality
lefthook install

# Create build directory
mkdir build && cd build

# Configure (Debug mode for development)
cmake .. -DCMAKE_BUILD_TYPE=Debug -G Ninja

# Build
cmake --build . -- -j$(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo cmake --install .
```

### Setting Up Git Hooks

Horcrux uses [Lefthook](https://github.com/evilmartians/lefthook) for managing git hooks. The hooks automatically:

**Pre-commit:**
- Format C++ code with clang-format
- Validate YAML files with yamllint
- Validate GitHub Actions with actionlint
- Check for merge conflicts and TODOs

**Pre-push:**
- Build the project
- Run tests

**Installing Lefthook:**

```bash
# On Linux/macOS
curl -fsSL https://raw.githubusercontent.com/evilmartians/lefthook/master/install.sh | bash

# Or download from releases
# https://github.com/evilmartians/lefthook/releases

# After installation, run in the repository root:
lefthook install
```

**Testing hooks without committing:**

```bash
# Test pre-commit hooks
lefthook run pre-commit

# Test pre-push hooks
lefthook run pre-push
```

**Skipping hooks (emergency only):**

```bash
# Skip all hooks for one commit
LEFTHOOK=0 git commit -m "emergency fix"

# Skip specific hook
LEFTHOOK_EXCLUDE=build-project git push
```

**Note:** Hooks are essential for code quality. Only skip them in true emergencies.

### Development Tips

- Use **Debug builds** during development for better error messages
- Enable **sanitizers** for catching bugs:
  ```bash
  cmake .. -DCMAKE_BUILD_TYPE=Debug \
           -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
  ```
- Use **compiler warnings** as errors to maintain code quality
- Run tests frequently to catch regressions early
- **Pre-commit hooks auto-format code** - stage files and commit, hooks will fix formatting
- If hooks fail, read the error messages and fix the issues before committing

## Coding Standards

**For comprehensive coding standards, see [docs/coding-standards.md](../docs/coding-standards.md).**

**For AI-assisted development, see [.github/copilot-instructions.md](.github/copilot-instructions.md).**

This document covers all aspects of C++23 development for Horcrux, including:
- Project-wide C++23 conventions (RAII, smart pointers, const correctness, error handling)
- Naming conventions and code organization
- Memory safety and concurrency policies
- Testing requirements (unit + integration + benchmarks)
- No global mutable state policy
- Threading and coroutine guidelines
- Performance optimization strategies
- Complete code review checklist

### Quick Reference

1. **Modern C++23** - Use modern C++ features appropriately
   - Prefer `std::expected` over exceptions for error handling
   - Use `constexpr` and `consteval` where applicable
   - Leverage concepts for template constraints
   - Use structured bindings and ranges

2. **Safety and Correctness**
   - **Zero undefined behavior** - all code must be well-defined
   - **RAII principles** - manage resources with automatic lifetime
   - **const-correctness** - mark everything const that can be
   - **Thread-safety** - document thread-safety guarantees

3. **Performance**
   - **Cache locality** - structure data for cache-friendly access
   - **Lock-free algorithms** where possible
   - **Avoid allocations** in hot paths
   - **Profile before optimizing**

4. **Code Style**
   - Use **snake_case** for variables and functions
   - Use **PascalCase** for types and classes
   - Use **SCREAMING_SNAKE_CASE** for macros (avoid macros when possible)
   - **Indent with 4 spaces** (no tabs)
   - **Max line length**: 100 characters
   - **Braces**: K&R style (opening brace on same line)

5. **Documentation**
   - Document all public APIs with clear comments
   - Explain **why**, not just **what** in complex code
   - Include usage examples for non-trivial APIs
   - Keep comments up-to-date with code changes

### Example Code Style

```cpp
// Good example
class BuildEngine {
public:
    // Constructs a build engine with the given configuration.
    // 
    // @param config The build configuration
    // @return Expected BuildEngine or error
    static std::expected<BuildEngine, Error> create(const Config& config);

    // Executes the build graph.
    //
    // @param graph The dependency graph to execute
    // @return Expected result or error
    auto execute(const DependencyGraph& graph) -> std::expected<BuildResult, Error>;

private:
    Config config_;
    std::unique_ptr<CacheManager> cache_;
};
```

## Testing Guidelines

### Test Requirements

- **All new features** must include tests
- **Bug fixes** should include a regression test
- **Tests must be fast** - use mocks/stubs for slow operations
- **Tests must be deterministic** - no flaky tests

### Test Structure

```cpp
#include <gtest/gtest.h>

TEST(BuildEngineTest, ExecutesSimpleGraph) {
    // Arrange
    auto config = Config::default_config();
    auto engine = BuildEngine::create(config).value();
    auto graph = create_simple_test_graph();

    // Act
    auto result = engine.execute(graph);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->status, BuildStatus::Success);
}
```

### Running Tests

```bash
# Run all tests
ctest --output-on-failure

# Run specific test
ctest -R BuildEngineTest

# Run with verbose output
ctest -V

# Run with parallel execution
ctest -j$(nproc)
```

## Commit Message Guidelines

We follow conventional commit format for clear history:

### Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, missing semicolons, etc.)
- `refactor`: Code refactoring without behavior change
- `perf`: Performance improvements
- `test`: Adding or updating tests
- `chore`: Build process, tooling, dependencies

### Examples

```
feat(cache): add distributed cache support

Implement distributed caching using Redis as backend.
This allows sharing build artifacts across machines.

Closes #123
```

```
fix(graph): resolve circular dependency detection

The previous algorithm had a bug where self-referencing
nodes weren't properly detected. This adds proper handling.

Fixes #456
```

### Guidelines

- Use **imperative mood** in subject line ("add" not "added")
- **Capitalize** first letter of subject
- **No period** at the end of subject
- **Limit subject** to 50 characters
- **Wrap body** at 72 characters
- Explain **what and why**, not how
- Reference issues with `Closes #123`, `Fixes #456`, or `Refs #789`

## Review Process

### What to Expect

1. **Automated checks** run on all PRs (build, tests, linting)
2. **Code review** by at least one maintainer
3. **Feedback** - reviewers may request changes
4. **Iteration** - address feedback and update the PR
5. **Approval** - once approved, a maintainer will merge

### Review Criteria

- **Correctness** - does the code do what it claims?
- **Tests** - are there adequate tests?
- **Documentation** - is it properly documented?
- **Style** - does it follow our coding standards?
- **Performance** - are there any performance concerns?
- **Design** - does it fit the overall architecture?

### Tips for Getting PRs Merged

- **Keep PRs focused** - one feature/fix per PR
- **Keep PRs small** - easier to review
- **Write good descriptions** - explain the why
- **Respond promptly** to review feedback
- **Be patient** - maintainers are often volunteers

## Community

### Getting Help

- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: Questions, ideas, and general discussion
- **Email**: For private matters, contact [maintainers@horcruxsys.org](mailto:maintainers@horcruxsys.org)

### Stay Updated

- Watch the repository for updates
- Star the project if you find it useful
- Follow our [roadmap](README.md#-roadmap) for upcoming features

### Recognition

Contributors are recognized in:
- The project's CONTRIBUTORS file (coming soon)
- Release notes for significant contributions
- Special mentions for milestone achievements

---

**Thank you for contributing to Horcrux!** Together we're building the future of build systems. 🚀

If you have questions about contributing, feel free to ask in [GitHub Discussions](https://github.com/horcruxsys/horcrux/discussions).
