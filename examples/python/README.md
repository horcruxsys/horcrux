# Python Example — Horcrux Polyglot Adapter

This example demonstrates building a Python library, binary, and test using the
Horcrux `python` adapter.

## Layout

```
examples/python/
├── BUILD              # Horcrux target definitions
└── src/
    ├── greet.py       # py_library
    ├── main.py        # py_binary
    └── greet_test.py  # py_test
```

## Targets

| Target        | Kind         | Description                         |
|---------------|--------------|-------------------------------------|
| `:greet`      | `py_library` | Greeting module                     |
| `:hello`      | `py_binary`  | CLI script using the greet library  |
| `:greet_test` | `py_test`    | unittest-based tests for greet      |

## Build

```bash
horcrux build //examples/python:hello
```

## Test

```bash
horcrux test //examples/python:greet_test
```

## Query

```bash
horcrux query //examples/python:all
horcrux query --deps //examples/python:hello
```

## Toolchain Setup

Horcrux detects the Python interpreter via:
1. `HORCRUX_PYTHON` environment variable (absolute path override).
2. `python3` on `PATH`.
3. `python` on `PATH`.

Run `horcrux doctor` to verify toolchain detection.
