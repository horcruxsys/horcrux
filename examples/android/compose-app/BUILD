# Horcrux BUILD file for compose-app
# Modern Jetpack Compose Android app

android_binary(
    name = "app",
    srcs = glob([
        "src/main/java/**/*.kt",
    ]),
    manifest = "src/main/AndroidManifest.xml",
    kotlin_options = {
        "jvmTarget": "17",
        "languageVersion": "1.9",
    },
    compose_options = {
        "kotlinCompilerExtensionVersion": "1.5.8",
    },
    deps = [
        "@maven//:androidx_core_core_ktx",
        "@maven//:androidx_lifecycle_lifecycle_runtime_ktx",
        "@maven//:androidx_activity_activity_compose",
        "@maven//:androidx_compose_ui_ui",
        "@maven//:androidx_compose_ui_ui_tooling_preview",
        "@maven//:androidx_compose_material3_material3",
        "@maven//:androidx_navigation_navigation_compose",
        "@maven//:androidx_lifecycle_lifecycle_viewmodel_compose",
    ],
    min_sdk_version = 24,
    target_sdk_version = 34,
    visibility = ["//visibility:public"],
)

android_apk(
    name = "debug",
    app = ":app",
    keystore = "//tools/keystore:debug_keystore",
    compose_metrics = True,
    compose_reports = True,
)

android_apk(
    name = "release",
    app = ":app",
    keystore = "//tools/keystore:release_keystore",
    proguard_specs = ["proguard-rules.pro"],
)
