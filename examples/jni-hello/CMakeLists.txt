# JNI Hello Example - CMakeLists.txt
# Demonstrates building JNI libraries for Android using Horcrux NDK compiler
# Copyright (C) 2025 Horcrux Project Contributors
# Licensed under the MIT License

cmake_minimum_required(VERSION 3.25)
project(HelloJNI VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find JNI headers (optional - can use NDK's JNI headers)
# This is typically provided by the Android NDK
# For this example, we'll assume the NDK provides the JNI headers

# Source files
set(SOURCES
    cpp/hello_jni.cpp
    cpp/math_utils.cpp
    cpp/math_jni.cpp
)

# Create shared library for each ABI
# This would typically be called with different toolchain files for each ABI

# Build native library
add_library(hello-jni SHARED ${SOURCES})

# Set library properties
set_target_properties(hello-jni PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Enable all warnings
target_compile_options(hello-jni PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
)

# Link with Android log library
target_link_libraries(hello-jni PRIVATE log)

# Installation rules
install(TARGETS hello-jni
    LIBRARY DESTINATION lib/${CMAKE_ANDROID_ARCH_ABI}
)
