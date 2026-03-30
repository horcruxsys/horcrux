# Rust Example — Horcrux Polyglot Adapter

This example demonstrates building a Rust library, binary, and test using the
Horcrux `rust` adapter.

## Layout

```
examples/rust/
├── BUILD              # Horcrux target definitions
└── src/
    ├── greet.rs       # rust_library
    ├── main.rs        # rust_binary
    └── greet_test.rs  # rust_test
```

## Targets

| Target        | Kind           | Description                        |
|---------------|----------------|------------------------------------|
| `:greet`      | `rust_library` | Greeting library crate (rlib)      |
| `:hello`      | `rust_binary`  | CLI binary using the greet library |
| `:greet_test` | `rust_test`    | Integration test for greet library |

## Build

```bash
horcrux build //examples/rust:hello
```

## Test

```bash
horcrux test //examples/rust:greet_test
```

## Query

```bash
# List all targets in this package
horcrux query //examples/rust:all

# Show direct dependencies of the binary
horcrux query --deps //examples/rust:hello
```

## Toolchain Setup

Horcrux detects `rustc` via:
1. `HORCRUX_RUSTC` environment variable (absolute path override).
2. `rustc` on `PATH`.

Install Rust via [rustup](https://rustup.rs/):

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

Run `horcrux doctor` to verify toolchain detection.
