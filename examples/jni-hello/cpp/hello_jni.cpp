// Hello JNI - Simple Android JNI Example
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <cstring>
#include <jni.h>
#include <string>

// JNI function to get a greeting message from native code
extern "C" JNIEXPORT jstring JNICALL
Java_com_horcrux_example_HelloJni_getGreeting(JNIEnv* env, jobject /* this */) {
  std::string greeting = "Hello from C++ via JNI!";
  return env->NewStringUTF(greeting.c_str());
}

// JNI function to perform a simple calculation
extern "C" JNIEXPORT jint JNICALL Java_com_horcrux_example_HelloJni_addNumbers(JNIEnv* /* env */,
                                                                               jobject /* this */,
                                                                               jint a, jint b) {
  return a + b;
}

// JNI function to demonstrate string manipulation
extern "C" JNIEXPORT jstring JNICALL
Java_com_horcrux_example_HelloJni_reverseString(JNIEnv* env, jobject /* this */, jstring input) {
  if (input == nullptr) {
    return env->NewStringUTF("");
  }

  const char* str = env->GetStringUTFChars(input, nullptr);
  if (str == nullptr) {
    return env->NewStringUTF("");
  }

  std::string reversed(str);
  std::reverse(reversed.begin(), reversed.end());

  env->ReleaseStringUTFChars(input, str);
  return env->NewStringUTF(reversed.c_str());
}

// JNI_OnLoad - Called when the native library is loaded
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* /* vm */, void* /* reserved */) {
  // Return JNI version
  return JNI_VERSION_1_6;
}
