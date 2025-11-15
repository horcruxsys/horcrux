# Horcrux Examples

This directory contains example projects demonstrating Horcrux build system usage.

## Examples

### Hello Example

A simple C++ application with a library dependency:

```bash
# Build the hello app
horcrux build //examples/hello:app

# Build just the library
horcrux build //examples/hello:lib
```

**Structure:**
- `main.cpp` - Application entry point
- `lib.cpp`, `lib.h` - Simple greeting library
- `BUILD` - Build configuration

### Simple Example

A minimal single-file application:

```bash
# Build the simple app
horcrux build //examples/simple:app
```

**Structure:**
- `main.cpp` - Application entry point
- `BUILD` - Build configuration

## Testing Cache Behavior

Run the same build command twice to see caching in action:

```bash
# First build (slower - compiles from source)
horcrux build //examples/hello:app

# Second build (instant - uses cache)
horcrux build //examples/hello:app
```

## Verbose Mode

Enable verbose logging to see detailed build information:

```bash
horcrux build //examples/hello:app --verbose
```
