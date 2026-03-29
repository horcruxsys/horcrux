# Java Adapter (non-Android)

The Horcrux Java adapter (`java`) implements build and test semantics for plain
JVM Java projects using a local JDK toolchain (`javac`, `jar`, `java`).

> **Note:** This adapter targets non-Android JVM workflows. For Android
> compilation, the existing Android pipeline components apply.

## Supported Target Kinds

| Kind           | Description                                       |
|----------------|---------------------------------------------------|
| `java_library` | Java sources compiled to `.class` files + JAR     |
| `java_binary`  | Executable JAR with a specified main class         |
| `java_test`    | JAR + test execution via a main class              |

## Target Attributes

| Attribute    | Kind(s)                        | Description                                      |
|--------------|--------------------------------|--------------------------------------------------|
| `srcs`       | all                            | List of `.java` source files                     |
| `deps`       | all                            | Labels of `java_library` dependencies            |
| `javacopts`  | all                            | Additional flags passed to `javac`               |
| `main_class` | `java_binary`, `java_test`     | Fully-qualified main class (e.g. `com.x.Main`)   |

## Build Pipeline

1. **Compile** — `javac -source <ver> -target <ver> -d <classes_dir> [-cp <classpath>] <srcs...>`
2. **Archive** — `jar cf <name>.jar -C <classes_dir> .` (uses `cfe` for `java_binary` to embed `Main-Class`)
3. **Test** (java_test only) — `java -cp <test.jar> <main_class>`

## Cache Key

The cache key is a deterministic SHA-256 hash of:
- Target label
- Target kind
- Java toolchain version string
- Source compatibility version
- Sorted list of source file paths
- `javacopts`

## Toolchain Detection

1. `HORCRUX_JAVAC` environment variable (absolute path override for `javac`).
2. `javac` on `PATH`.

Returns `AdapterError::ToolchainError` if no `javac` is found.

## Example

```
# examples/java/BUILD
java_library(
    name = "greeter",
    srcs = ["src/com/example/Greeter.java"],
)

java_binary(
    name = "hello",
    srcs = ["src/com/example/Main.java"],
    main_class = "com.example.Main",
    deps = [":greeter"],
)

java_test(
    name = "greeter_test",
    srcs = ["src/com/example/GreeterTest.java"],
    main_class = "com.example.GreeterTest",
    deps = [":greeter"],
)
```

```bash
horcrux build //examples/java:hello
horcrux test  //examples/java:greeter_test
horcrux query --deps //examples/java:hello
```

## Troubleshooting

- **"Toolchain not found or misconfigured"** — Install a JDK (OpenJDK 17 recommended):
  ```bash
  # Debian/Ubuntu
  sudo apt-get install -y openjdk-17-jdk

  # macOS (Homebrew)
  brew install openjdk@17
  ```
  Then run `horcrux doctor`.

- **Classpath errors at test time** — Ensure all `deps` are declared and that
  the test's `main_class` attribute points to the correct test harness class.
