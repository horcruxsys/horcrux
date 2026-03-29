# Python Adapter

The Horcrux Python adapter (`python`) implements build and test semantics for
Python modules and scripts using a local Python 3 interpreter.

## Supported Target Kinds

| Kind         | Description                                    |
|--------------|------------------------------------------------|
| `py_library` | Python module(s); validated with `py_compile`  |
| `py_binary`  | Python script entrypoint                       |
| `py_test`    | Python unittest-based test target              |

## Target Attributes

| Attribute | Kind(s)                | Description                                        |
|-----------|------------------------|----------------------------------------------------|
| `srcs`    | all                    | List of `.py` source files                         |
| `deps`    | all                    | Labels of `py_library` dependencies                |
| `main`    | `py_binary`, `py_test` | Main `.py` file; defaults to first entry in `srcs` |

## Cache Key

The cache key is a deterministic SHA-256 hash of:
- Target label
- Target kind
- Python interpreter version string
- Sorted list of source file paths

## Toolchain Detection

1. `HORCRUX_PYTHON` environment variable (absolute path override).
2. `python3` on `PATH`.
3. `python` on `PATH`.

Returns `AdapterError::ToolchainError` if no Python interpreter is found.

## Example

```
# examples/python/BUILD
py_library(
    name = "greet",
    srcs = ["src/greet.py"],
)

py_binary(
    name = "hello",
    srcs = ["src/main.py"],
    main = "src/main.py",
    deps = [":greet"],
)

py_test(
    name = "greet_test",
    srcs = ["src/greet_test.py"],
    main = "src/greet_test.py",
    deps = [":greet"],
)
```

```bash
horcrux build //examples/python:hello
horcrux test  //examples/python:greet_test
horcrux query --deps //examples/python:hello
```

## Troubleshooting

- **"Toolchain not found or misconfigured"** — Install Python 3:
  ```bash
  # Debian/Ubuntu
  sudo apt-get install -y python3

  # macOS (Homebrew)
  brew install python3
  ```
  Then run `horcrux doctor`.

- **`py_test` fails to discover tests** — Ensure your test file uses the
  `unittest` module and is listed in `srcs` and `main`.
