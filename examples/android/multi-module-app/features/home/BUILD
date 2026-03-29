# Horcrux BUILD file for features/home module

android_library(
    name = "home",
    srcs = glob([
        "src/main/java/**/*.java",
    ]),
    manifest = "src/main/AndroidManifest.xml",
    resource_files = glob([
        "src/main/res/**",
    ]),
    deps = [
        "//examples/android/multi-module-app/libraries/network:network",
        "//examples/android/multi-module-app/libraries/utils:utils",
        "@maven//:androidx_appcompat_appcompat",
    ],
    visibility = ["//visibility:public"],
)
