// Math JNI Bindings - Native math functions exposed to Java
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <jni.h>
#include "math_utils.h"

using namespace horcrux::examples;

// JNI function to calculate factorial
extern "C" JNIEXPORT jint JNICALL
Java_com_horcrux_example_MathUtils_factorial(JNIEnv* /* env */, jobject /* this */, jint n) {
    return factorial(n);
}

// JNI function to check if a number is prime
extern "C" JNIEXPORT jboolean JNICALL
Java_com_horcrux_example_MathUtils_isPrime(JNIEnv* /* env */, jobject /* this */, jint n) {
    return is_prime(n) ? JNI_TRUE : JNI_FALSE;
}

// JNI function to calculate Fibonacci number
extern "C" JNIEXPORT jint JNICALL
Java_com_horcrux_example_MathUtils_fibonacci(JNIEnv* /* env */, jobject /* this */, jint n) {
    return fibonacci(n);
}
