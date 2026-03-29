# Horcrux BUILD file for basic-xml-app
# Traditional Android app with XML layouts

android_binary(
    name = "app",
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
