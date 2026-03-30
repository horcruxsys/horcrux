# examples/python/BUILD
# Horcrux BUILD file for the Python example project.

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
