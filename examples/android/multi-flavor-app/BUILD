# Horcrux BUILD file for multi-flavor-app
# Android app with product flavors (free, paid) and build types (debug, release)

# Shared base library
android_library(
    name = "app_base",
    srcs = glob([
        "src/main/java/**/*.java",
    ]),
    manifest = "src/main/AndroidManifest.xml",
    resource_files = glob([
        "src/main/res/**",
    ]),
    deps = [
        "@maven//:androidx_appcompat_appcompat",
        "@maven//:androidx_constraintlayout_constraintlayout",
        "@maven//:com_google_android_material_material",
    ],
    visibility = ["//visibility:private"],
)

# Free flavor library (overlays base)
android_library(
    name = "app_free",
    srcs = glob([
        "src/free/java/**/*.java",
    ]),
    resource_files = glob([
        "src/free/res/**",
    ]),
    deps = [":app_base"],
    visibility = ["//visibility:private"],
)

# Paid flavor library (overlays base)
android_library(
    name = "app_paid",
    srcs = glob([
        "src/paid/java/**/*.java",
    ]),
    resource_files = glob([
        "src/paid/res/**",
    ]),
    deps = [":app_base"],
    visibility = ["//visibility:private"],
)

# Variant targets: freeDebug, freeRelease, paidDebug, paidRelease
android_binary(
    name = "freeDebug",
    flavor = "free",
    build_type = "debug",
    manifest = "src/main/AndroidManifest.xml",
    application_id = "com.example.flavor.free.debug",
    version_code = 1,
    deps = [":app_free"],
    min_sdk_version = 21,
    target_sdk_version = 34,
    visibility = ["//visibility:public"],
)

android_binary(
    name = "freeRelease",
    flavor = "free",
    build_type = "release",
    manifest = "src/main/AndroidManifest.xml",
    application_id = "com.example.flavor.free",
    version_code = 1,
    deps = [":app_free"],
    proguard_specs = ["proguard-rules.pro"],
    min_sdk_version = 21,
    target_sdk_version = 34,
    visibility = ["//visibility:public"],
)

android_binary(
    name = "paidDebug",
    flavor = "paid",
    build_type = "debug",
    manifest = "src/main/AndroidManifest.xml",
    application_id = "com.example.flavor.paid.debug",
    version_code = 100,
    deps = [":app_paid"],
    min_sdk_version = 21,
    target_sdk_version = 34,
    visibility = ["//visibility:public"],
)

android_binary(
    name = "paidRelease",
    flavor = "paid",
    build_type = "release",
    manifest = "src/main/AndroidManifest.xml",
    application_id = "com.example.flavor.paid",
    version_code = 100,
    deps = [":app_paid"],
    proguard_specs = ["proguard-rules.pro"],
    min_sdk_version = 21,
    target_sdk_version = 34,
    visibility = ["//visibility:public"],
)
