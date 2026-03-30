# Java Example — Horcrux Polyglot Adapter

This example demonstrates building a Java library, binary, and test using the
Horcrux `java` adapter (non-Android JVM workflow).

## Layout

```
examples/java/
├── BUILD                             # Horcrux target definitions
└── src/com/example/
    ├── Greeter.java                  # java_library
    ├── Main.java                     # java_binary
    └── GreeterTest.java              # java_test
```

## Targets

| Target          | Kind           | Description                          |
|-----------------|----------------|--------------------------------------|
| `:greeter`      | `java_library` | Greeter class library (produces JAR) |
| `:hello`        | `java_binary`  | CLI binary using the Greeter library |
| `:greeter_test` | `java_test`    | Standalone test for Greeter class    |

## Build

```bash
horcrux build //examples/java:hello
```

## Test

```bash
horcrux test //examples/java:greeter_test
```

## Query

```bash
horcrux query //examples/java:all
horcrux query --deps //examples/java:hello
```

## Toolchain Setup

Horcrux detects the Java toolchain via:
1. `HORCRUX_JAVAC` environment variable (absolute path override for `javac`).
2. `javac` on `PATH`.

Install a JDK (e.g., OpenJDK 17):

```bash
# Debian / Ubuntu
sudo apt-get install -y openjdk-17-jdk

# macOS (Homebrew)
brew install openjdk@17
```

Run `horcrux doctor` to verify toolchain detection.
