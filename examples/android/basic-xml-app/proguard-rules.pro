# ProGuard rules for basic-xml-app release build
# Keep Application class and Activities
-keep class com.example.basicxml.** { *; }

# Keep AndroidX and Material components
-dontwarn androidx.**
-keep class androidx.** { *; }
