# Continuous Integration

This document describes the CI/CD pipeline for the Horcrux build system.

## Overview

Horcrux uses GitHub Actions for continuous integration. The CI pipeline ensures that all code changes are automatically tested, linted, and validated before merging.

## Workflows

### Build and Test Workflow

**File:** `.github/workflows/build.yml`

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests targeting `main` or `develop` branches  
- Manual workflow dispatch

**Jobs:**

#### 1. Lint Job
Runs code quality checks including:
- **clang-format**: Ensures consistent code formatting
- **clang-tidy**: Performs static analysis for common issues

This job runs on Ubuntu 22.04 and uses Clang 17.

#### 2. Build Job
Builds the project with multiple compiler configurations:
- **Compilers:** GCC 13, Clang 17
- **Operating Systems:** Ubuntu 22.04, Ubuntu 24.04
- **Build Type:** Release

Features:
- **CMake configuration** with C++23 support
- **ccache** for faster subsequent builds
- **Ninja** build system for optimal performance
- **Dependency caching** via GitHub Actions cache
- **Build artifact upload** for debugging (Clang 17 on Ubuntu 22.04)

#### 3. Status Job
Final status job that reports the overall CI result.

## CI Badge

The CI status badge is displayed in the README:

```markdown
[![Build Status](https://github.com/horcruxsys/horcrux/actions/workflows/build.yml/badge.svg)](https://github.com/horcruxsys/horcrux/actions/workflows/build.yml)
```

## Performance

**Target:** CI builds should complete in under 5 minutes.

**Optimizations:**
- Parallel builds with `-j $(nproc)`
- ccache for compilation caching
- GitHub Actions cache for dependencies
- Ninja build system
- Concurrent workflow cancellation for outdated runs

## Early-Stage Behavior

Since Horcrux is in early development, the CI workflow is designed to:
- Gracefully handle missing source files
- Skip checks when no C++ code exists
- Automatically enable full builds once CMakeLists.txt is added
- Be future-proof for when tests and source code are added

This ensures CI passes during the bootstrapping phase while maintaining full validation once development progresses.

## Local Testing

To replicate CI checks locally:

### 1. Code Formatting
```bash
# Check formatting
find . -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
  -not -path "./build/*" | xargs clang-format-17 --dry-run --Werror

# Fix formatting
find . -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
  -not -path "./build/*" | xargs clang-format-17 -i
```

### 2. Build Locally
```bash
# Configure
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++-17

# Build
cmake --build . -j $(nproc)

# Test
ctest --output-on-failure
```

### 3. Static Analysis
```bash
# Run clang-tidy (requires compile_commands.json)
clang-tidy-17 src/**/*.cpp --config-file=.clang-tidy
```

## Troubleshooting

### Workflow Not Running
Workflows only run when:
1. Committed to a branch with the workflow file
2. Pushed to GitHub
3. Targeting a branch specified in the `on:` triggers

### Build Failures
Check the workflow logs:
1. Go to Actions tab in GitHub
2. Click on the failed workflow run
3. Review job logs for errors

### Slow Builds
If builds exceed 5 minutes:
- Check cache hit rate
- Review parallel job count
- Consider splitting build matrix further

## Future Enhancements

Planned improvements:
- [ ] Cross-platform builds (macOS, Windows)
- [ ] Integration tests
- [ ] Coverage reporting
- [ ] Performance benchmarking
- [ ] Sanitizer runs (ASan, UBSan, TSan)
- [ ] Release automation
