# Hello World Example

This is a simple "Hello World" example that demonstrates building a basic C++ binary target with Horcrux.

## Building

```bash
horcrux build //examples/hello:hello
```

## Running

After building, run the binary:

```bash
./bazel-bin/examples/hello/hello
```

## Expected Output

```
Hello from Horcrux Build System!
This is a simple example target.
```

## Purpose

This example serves as:
- A basic integration test target
- A demonstration of the cc_binary rule
- A validation that the build system can compile and link C++ code
- A benchmark target for measuring build performance
