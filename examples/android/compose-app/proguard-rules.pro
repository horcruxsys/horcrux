# ProGuard rules for compose-app release build
-dontwarn androidx.compose.**
-keep class androidx.compose.** { *; }
-keepclassmembers class * {
    @androidx.compose.runtime.Composable *;
}
