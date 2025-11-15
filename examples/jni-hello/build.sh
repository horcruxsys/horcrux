#!/bin/bash
# Build JNI Hello Example for All Android ABIs
# Copyright (C) 2025 Horcrux Project Contributors
# Licensed under the MIT License

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building JNI Hello Example for All Android ABIs${NC}"
echo "=============================================="
echo ""

# Check if NDK is available
if [ -z "$ANDROID_NDK_ROOT" ] && [ -z "$ANDROID_NDK_HOME" ]; then
    echo -e "${RED}Error: ANDROID_NDK_ROOT or ANDROID_NDK_HOME not set${NC}"
    echo "Please set one of these environment variables to your NDK installation path"
    exit 1
fi

# Use ANDROID_NDK_ROOT if set, otherwise use ANDROID_NDK_HOME
NDK_ROOT="${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
echo -e "${GREEN}Using NDK:${NC} $NDK_ROOT"
echo ""

# Define ABIs to build
ABIS=("arm64-v8a" "armeabi-v7a" "x86" "x86_64")

# API level
API_LEVEL=${ANDROID_API_LEVEL:-21}
echo -e "${GREEN}Target API Level:${NC} $API_LEVEL"
echo ""

# Output directory
OUTPUT_DIR="$(pwd)/build/libs"
mkdir -p "$OUTPUT_DIR"

# Build for each ABI
for ABI in "${ABIS[@]}"; do
    echo -e "${YELLOW}Building for ABI: $ABI${NC}"
    
    # Create build directory for this ABI
    BUILD_DIR="build/$ABI"
    mkdir -p "$BUILD_DIR"
    
    # Run Horcrux NDK compiler to generate CMake toolchain file
    # This would be: horcrux-cli ndk generate-toolchain --abi $ABI --api-level $API_LEVEL --output $BUILD_DIR/toolchain.cmake
    # For now, we'll use the NDK's own toolchain file
    
    # Configure CMake with NDK toolchain
    cmake -S . -B "$BUILD_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$NDK_ROOT/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_PLATFORM="android-$API_LEVEL" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$OUTPUT_DIR"
    
    # Build
    cmake --build "$BUILD_DIR" --config Release -j$(nproc)
    
    # Install to output directory
    cmake --install "$BUILD_DIR" --prefix "$OUTPUT_DIR"
    
    echo -e "${GREEN}✓ Built $ABI successfully${NC}"
    echo ""
done

echo -e "${GREEN}Build Complete!${NC}"
echo "=============================================="
echo "Libraries installed to: $OUTPUT_DIR"
echo ""
echo "Directory structure:"
tree "$OUTPUT_DIR" 2>/dev/null || find "$OUTPUT_DIR" -type f
