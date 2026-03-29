# Horcrux BUILD file for multi-module-app (root)
# Aggregates all modules; build the full app from here.

# Build the main application
android_binary(
    name = "app",
    srcs = glob([
        "app/src/main/java/**/*.java",
        "app/src/main/java/**/*.kt",
    ]),
    manifest = "app/src/main/AndroidManifest.xml",
    resource_files = glob([
        "app/src/main/res/**",
    ]),
    deps = [
        "//examples/android/multi-module-app/features/home:home",
        "//examples/android/multi-module-app/libraries/network:network",
        "//examples/android/multi-module-app/libraries/utils:utils",
        "@maven//:androidx_appcompat_appcompat",
        "@maven//:androidx_core_core_ktx",
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
    proguard_specs = ["app/proguard-rules.pro"],
)
