# examples/rust/BUILD
# Horcrux BUILD file for the Rust example project.

rust_library(
    name = "greet",
    srcs = ["src/greet.rs"],
    edition = "2021",
)

rust_binary(
    name = "hello",
    srcs = ["src/main.rs"],
    deps = [":greet"],
    edition = "2021",
)

rust_test(
    name = "greet_test",
    srcs = ["src/greet_test.rs"],
    deps = [":greet"],
    edition = "2021",
)
