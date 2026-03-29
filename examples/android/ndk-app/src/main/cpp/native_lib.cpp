// Horcrux NDK App - native_lib.cpp
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <cstring>
#include <jni.h>
#include <string>

extern "C" {

JNIEXPORT jstring JNICALL Java_com_example_ndk_MainActivity_getNativeGreeting(JNIEnv* env,
                                                                              jobject /* this */) {
  std::string greeting = "Hello from C++! (built with Horcrux NDK support)";
  return env->NewStringUTF(greeting.c_str());
}

JNIEXPORT jint JNICALL Java_com_example_ndk_MainActivity_multiplyNative(JNIEnv* /* env */,
                                                                        jobject /* this */, jint a,
                                                                        jint b) {
  return a * b;
}

} // extern "C"
