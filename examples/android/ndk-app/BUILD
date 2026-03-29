# Horcrux BUILD file for ndk-app
# Android app with native C++ code (JNI)

# Native shared library (built per ABI)
cc_library(
    name = "native_lib",
    srcs = glob(["src/main/cpp/**/*.cpp"]),
    hdrs = glob(["src/main/cpp/**/*.h"]),
    copts = [
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-ffast-math",
    ],
    linkopts = ["-llog"],
    target_compatible_with = ["@platforms//os:android"],
    visibility = ["//visibility:public"],
)

android_binary(
    name = "app",
    srcs = glob([
        "src/main/java/**/*.java",
        "src/main/java/**/*.kt",
    ]),
    manifest = "src/main/AndroidManifest.xml",
    resource_files = glob([
        "src/main/res/**",
    ]),
    native_libs = [":native_lib"],
    ndk_abis = [
        "arm64-v8a",
        "armeabi-v7a",
        "x86_64",
    ],
    deps = [
        "@maven//:androidx_appcompat_appcompat",
        "@maven//:androidx_constraintlayout_constraintlayout",
    ],
    min_sdk_version = 21,
    target_sdk_version = 34,
    visibility = ["//visibility:public"],
)

android_apk(
    name = "debug",
    app = ":app",
    keystore = "//tools/keystore:debug_keystore",
)

android_apk(
    name = "release",
    app = ":app",
    keystore = "//tools/keystore:release_keystore",
    proguard_specs = ["proguard-rules.pro"],
)
