# ProGuard rules for multi-flavor-app release build
-keep class com.example.flavor.** { *; }
-dontwarn androidx.**
-keep class androidx.** { *; }
