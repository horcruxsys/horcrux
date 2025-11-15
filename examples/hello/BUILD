# Example BUILD file for hello application
# This is a demonstration of Horcrux build syntax

cc_library(
    name = "lib",
    srcs = ["lib.cpp", "lib.h"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "app",
    srcs = ["main.cpp"],
    deps = [":lib"],
)
# Horcrux BUILD file for hello example
# This is a simple cc_binary target that demonstrates basic build functionality

cc_binary(
    name = "hello",
    srcs = ["main.cpp"],
    visibility = ["//visibility:public"],
)
