# ProGuard rules for ndk-app release build
-keep class com.example.ndk.** { *; }
-keepclasseswithmembernames class * {
    native <methods>;
}
