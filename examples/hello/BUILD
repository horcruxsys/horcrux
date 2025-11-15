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
