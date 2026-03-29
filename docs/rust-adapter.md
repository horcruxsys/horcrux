# Rust Adapter

The Horcrux Rust adapter (`rust`) implements build and test semantics for Rust
crates using a local `rustc` toolchain.

## Supported Target Kinds

| Kind            | Description                            |
|-----------------|----------------------------------------|
| `rust_library`  | Rust crate compiled to `.rlib`         |
| `rust_binary`   | Rust crate compiled to an executable   |
| `rust_test`     | Rust test binary (`--test` mode)       |

## Target Attributes

| Attribute      | Kind(s)                         | Description                                        |
|----------------|---------------------------------|----------------------------------------------------|
| `srcs`         | all                             | List of `.rs` source files                         |
| `deps`         | all                             | Labels of other `rust_library` dependencies        |
| `edition`      | all                             | Rust edition (`2015`, `2018`, `2021`); default: `2021` |
| `rustc_flags`  | all                             | Additional flags passed to `rustc`                 |

## Cache Key

The cache key is a deterministic SHA-256 hash of:
- Target label
- Target kind
- `rustc` version string
- Sorted list of source file paths
- `rustc_flags`

## Toolchain Detection

1. `HORCRUX_RUSTC` environment variable (absolute path override).
2. `rustc` on `PATH`.

Returns `AdapterError::ToolchainError` if no `rustc` is found.

## Example

```
# examples/rust/BUILD
rust_library(
    name = "greet",
    srcs = ["src/greet.rs"],
    edition = "2021",
)

rust_binary(
    name = "hello",
    srcs = ["src/main.rs"],
    deps = [":greet"],
)

rust_test(
    name = "greet_test",
    srcs = ["src/greet_test.rs"],
    deps = [":greet"],
)
```

```bash
horcrux build //examples/rust:hello
horcrux test  //examples/rust:greet_test
horcrux query --deps //examples/rust:hello
```

## Troubleshooting

- **"Toolchain not found or misconfigured"** — Install Rust via `rustup`:
  ```bash
  curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
  ```
  Then run `horcrux doctor`.

- **Edition mismatch** — Explicitly set `edition = "2021"` (or the edition used
  in your crate) in the target.
