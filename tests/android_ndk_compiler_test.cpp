// Horcrux - Android NDK Compiler Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/core/android_ndk_compiler.h"

namespace horcrux::core::test {

// Helper to create a temporary directory for testing
class TempDir {
public:
  TempDir() {
    path_ = std::filesystem::temp_directory_path() / "horcrux_ndk_test";
    std::filesystem::remove_all(path_); // Clean up if exists
    std::filesystem::create_directories(path_);
  }

  ~TempDir() {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }

  auto path() const -> const std::filesystem::path& {
    return path_;
  }

private:
  std::filesystem::path path_;
};

// Test ABI string conversions
TEST(AndroidNdkCompilerTest, AbiToString) {
  EXPECT_EQ(to_string(AndroidAbi::Arm64V8a), "arm64-v8a");
  EXPECT_EQ(to_string(AndroidAbi::ArmeabiV7a), "armeabi-v7a");
  EXPECT_EQ(to_string(AndroidAbi::X86), "x86");
  EXPECT_EQ(to_string(AndroidAbi::X86_64), "x86_64");
}

TEST(AndroidNdkCompilerTest, AbiFromString) {
  EXPECT_EQ(abi_from_string("arm64-v8a"), AndroidAbi::Arm64V8a);
  EXPECT_EQ(abi_from_string("armeabi-v7a"), AndroidAbi::ArmeabiV7a);
  EXPECT_EQ(abi_from_string("x86"), AndroidAbi::X86);
  EXPECT_EQ(abi_from_string("x86_64"), AndroidAbi::X86_64);
  EXPECT_FALSE(abi_from_string("invalid").has_value());
}

TEST(AndroidNdkCompilerTest, GetAllAbis) {
  auto abis = get_all_abis();
  EXPECT_EQ(abis.size(), 4);
  EXPECT_TRUE(std::find(abis.begin(), abis.end(), AndroidAbi::Arm64V8a) != abis.end());
  EXPECT_TRUE(std::find(abis.begin(), abis.end(), AndroidAbi::ArmeabiV7a) != abis.end());
  EXPECT_TRUE(std::find(abis.begin(), abis.end(), AndroidAbi::X86) != abis.end());
  EXPECT_TRUE(std::find(abis.begin(), abis.end(), AndroidAbi::X86_64) != abis.end());
}

// Test error to string conversion
TEST(AndroidNdkCompilerTest, ErrorToString) {
  EXPECT_FALSE(to_string(NdkCompilerError::NdkNotFound).empty());
  EXPECT_FALSE(to_string(NdkCompilerError::ToolchainNotFound).empty());
  EXPECT_FALSE(to_string(NdkCompilerError::UnsupportedAbi).empty());
  EXPECT_FALSE(to_string(NdkCompilerError::CompilationFailed).empty());
}

// Test compiler creation with mock NDK
TEST(AndroidNdkCompilerTest, CreateCompilerWithInvalidNdk) {
  AndroidNdk ndk;
  ndk.path = "/nonexistent/ndk";
  ndk.version = "26.0.0";

  auto result = AndroidNdkCompiler::create(ndk, AndroidAbi::Arm64V8a);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), NdkCompilerError::ToolchainNotFound);
}

// Test compiler creation requires valid NDK structure
TEST(AndroidNdkCompilerTest, CreateCompilerRequiresToolchainDir) {
  TempDir temp;

  AndroidNdk ndk;
  ndk.path = temp.path();
  ndk.version = "26.0.0";

  // Without toolchains directory
  auto result = AndroidNdkCompiler::create(ndk, AndroidAbi::Arm64V8a);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), NdkCompilerError::ToolchainNotFound);
}

TEST(AndroidNdkCompilerTest, CreateCompilerRequiresSysroot) {
  TempDir temp;

  // Create minimal NDK structure without sysroot
  std::filesystem::create_directories(temp.path() / "toolchains" / "llvm" / "prebuilt" /
                                      "linux-x86_64" / "bin");

  AndroidNdk ndk;
  ndk.path = temp.path();
  ndk.version = "26.0.0";

  auto result = AndroidNdkCompiler::create(ndk, AndroidAbi::Arm64V8a);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), NdkCompilerError::SysrootNotFound);
}

TEST(AndroidNdkCompilerTest, CreateCompilerRequiresClang) {
  TempDir temp;

  // Create minimal NDK structure with sysroot but no clang
  auto toolchain_root = temp.path() / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64";
  std::filesystem::create_directories(toolchain_root / "bin");
  std::filesystem::create_directories(toolchain_root / "sysroot" / "usr" / "include");

  AndroidNdk ndk;
  ndk.path = temp.path();
  ndk.version = "26.0.0";

  auto result = AndroidNdkCompiler::create(ndk, AndroidAbi::Arm64V8a);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), NdkCompilerError::ClangNotFound);
}

// Test compile options validation
TEST(AndroidNdkCompilerTest, CompileOptionsDefaults) {
  NdkCompileOptions opts;
  EXPECT_EQ(opts.optimization_level, "-O2");
  EXPECT_FALSE(opts.debug_info);
  EXPECT_TRUE(opts.pic);
  EXPECT_TRUE(opts.exceptions);
  EXPECT_TRUE(opts.rtti);
  EXPECT_EQ(opts.cpp_std, "c++17");
}

// Test link options validation
TEST(AndroidNdkCompilerTest, LinkOptionsDefaults) {
  NdkLinkOptions opts;
  EXPECT_FALSE(opts.strip_symbols);
  EXPECT_TRUE(opts.shared);
}

// Test CMake toolchain file generation
TEST(AndroidNdkCompilerTest, GenerateCMakeToolchainFileRequiresValidNdk) {
  TempDir temp;

  // Create a complete mock NDK structure
  auto toolchain_root = temp.path() / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64";
  std::filesystem::create_directories(toolchain_root / "bin");
  std::filesystem::create_directories(toolchain_root / "sysroot" / "usr" / "include");

  // Create mock clang
  std::ofstream clang(toolchain_root / "bin" / "clang");
  clang << "#!/bin/bash\n";
  clang.close();

  AndroidNdk ndk;
  ndk.path = temp.path();
  ndk.version = "26.0.0";

  auto compiler_result = AndroidNdkCompiler::create(ndk, AndroidAbi::Arm64V8a, "21");
  ASSERT_TRUE(compiler_result.has_value());

  auto& compiler = *compiler_result;

  // Generate toolchain file
  auto toolchain_file = temp.path() / "android-arm64-v8a.cmake";
  auto result = compiler.generate_cmake_toolchain_file(toolchain_file);

  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(std::filesystem::exists(toolchain_file));

  // Verify file content
  std::ifstream file(toolchain_file);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("CMAKE_SYSTEM_NAME Android"), std::string::npos);
  EXPECT_NE(content.find("CMAKE_SYSTEM_VERSION 21"), std::string::npos);
  EXPECT_NE(content.find("CMAKE_ANDROID_ARCH_ABI arm64-v8a"), std::string::npos);
  EXPECT_NE(content.find("CMAKE_C_COMPILER"), std::string::npos);
  EXPECT_NE(content.find("CMAKE_CXX_COMPILER"), std::string::npos);
}

// Test detect all toolchains
TEST(AndroidNdkCompilerTest, DetectAllToolchainsRequiresValidNdk) {
  AndroidNdk ndk;
  ndk.path = "/nonexistent/ndk";
  ndk.version = "26.0.0";

  auto result = detect_all_ndk_toolchains(ndk);
  EXPECT_FALSE(result.has_value());
}

// Test compilation validation
TEST(AndroidNdkCompilerTest, CompileRequiresSourceFile) {
  TempDir temp;

  // Create minimal NDK structure
  auto toolchain_root = temp.path() / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64";
  std::filesystem::create_directories(toolchain_root / "bin");
  std::filesystem::create_directories(toolchain_root / "sysroot" / "usr" / "include");

  // Create mock clang
  std::ofstream clang(toolchain_root / "bin" / "clang");
  clang << "#!/bin/bash\n";
  clang.close();

  AndroidNdk ndk;
  ndk.path = temp.path();
  ndk.version = "26.0.0";

  auto compiler_result = AndroidNdkCompiler::create(ndk, AndroidAbi::Arm64V8a, "21");
  ASSERT_TRUE(compiler_result.has_value());

  auto& compiler = *compiler_result;

  // Try to compile non-existent file
  auto result = compiler.compile("/nonexistent/source.cpp", temp.path() / "output.o");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), NdkCompilerError::InvalidSourceFile);
}

// Test ABI toolchain properties
TEST(AndroidNdkCompilerTest, Arm64V8aToolchainProperties) {
  TempDir temp;

  // Create complete mock NDK structure
  auto toolchain_root = temp.path() / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64";
  std::filesystem::create_directories(toolchain_root / "bin");
  std::filesystem::create_directories(toolchain_root / "sysroot" / "usr" / "include");
  std::filesystem::create_directories(toolchain_root / "sysroot" / "usr" / "include" /
                                      "aarch64-linux-android");
  std::filesystem::create_directories(toolchain_root / "include" / "c++" / "v1");

  // Create mock clang
  std::ofstream clang(toolchain_root / "bin" / "clang");
  clang << "#!/bin/bash\n";
  clang.close();

  AndroidNdk ndk;
  ndk.path = temp.path();
  ndk.version = "26.0.0";

  auto compiler_result = AndroidNdkCompiler::create(ndk, AndroidAbi::Arm64V8a, "21");
  ASSERT_TRUE(compiler_result.has_value());

  auto& compiler = *compiler_result;
  const auto& toolchain = compiler.get_toolchain();

  EXPECT_EQ(toolchain.abi, AndroidAbi::Arm64V8a);
  EXPECT_EQ(toolchain.target_triple, "aarch64-linux-android");
  EXPECT_EQ(toolchain.api_level, "21");
  EXPECT_FALSE(toolchain.include_paths.empty());
}

TEST(AndroidNdkCompilerTest, ArmeabiV7aToolchainProperties) {
  TempDir temp;

  // Create complete mock NDK structure
  auto toolchain_root = temp.path() / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64";
  std::filesystem::create_directories(toolchain_root / "bin");
  std::filesystem::create_directories(toolchain_root / "sysroot" / "usr" / "include");

  // Create mock clang
  std::ofstream clang(toolchain_root / "bin" / "clang");
  clang << "#!/bin/bash\n";
  clang.close();

  AndroidNdk ndk;
  ndk.path = temp.path();
  ndk.version = "26.0.0";

  auto compiler_result = AndroidNdkCompiler::create(ndk, AndroidAbi::ArmeabiV7a, "21");
  ASSERT_TRUE(compiler_result.has_value());

  auto& compiler = *compiler_result;
  const auto& toolchain = compiler.get_toolchain();

  EXPECT_EQ(toolchain.abi, AndroidAbi::ArmeabiV7a);
  EXPECT_EQ(toolchain.target_triple, "armv7a-linux-androideabi");
  EXPECT_EQ(toolchain.api_level, "21");
}

TEST(AndroidNdkCompilerTest, X86ToolchainProperties) {
  TempDir temp;

  // Create complete mock NDK structure
  auto toolchain_root = temp.path() / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64";
  std::filesystem::create_directories(toolchain_root / "bin");
  std::filesystem::create_directories(toolchain_root / "sysroot" / "usr" / "include");

  // Create mock clang
  std::ofstream clang(toolchain_root / "bin" / "clang");
  clang << "#!/bin/bash\n";
  clang.close();

  AndroidNdk ndk;
  ndk.path = temp.path();
  ndk.version = "26.0.0";

  auto compiler_result = AndroidNdkCompiler::create(ndk, AndroidAbi::X86, "21");
  ASSERT_TRUE(compiler_result.has_value());

  auto& compiler = *compiler_result;
  const auto& toolchain = compiler.get_toolchain();

  EXPECT_EQ(toolchain.abi, AndroidAbi::X86);
  EXPECT_EQ(toolchain.target_triple, "i686-linux-android");
  EXPECT_EQ(toolchain.api_level, "21");
}

TEST(AndroidNdkCompilerTest, X86_64ToolchainProperties) {
  TempDir temp;

  // Create complete mock NDK structure
  auto toolchain_root = temp.path() / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64";
  std::filesystem::create_directories(toolchain_root / "bin");
  std::filesystem::create_directories(toolchain_root / "sysroot" / "usr" / "include");

  // Create mock clang
  std::ofstream clang(toolchain_root / "bin" / "clang");
  clang << "#!/bin/bash\n";
  clang.close();

  AndroidNdk ndk;
  ndk.path = temp.path();
  ndk.version = "26.0.0";

  auto compiler_result = AndroidNdkCompiler::create(ndk, AndroidAbi::X86_64, "21");
  ASSERT_TRUE(compiler_result.has_value());

  auto& compiler = *compiler_result;
  const auto& toolchain = compiler.get_toolchain();

  EXPECT_EQ(toolchain.abi, AndroidAbi::X86_64);
  EXPECT_EQ(toolchain.target_triple, "x86_64-linux-android");
  EXPECT_EQ(toolchain.api_level, "21");
}

// Test compile options building
TEST(AndroidNdkCompilerTest, CompileOptionsWithDefines) {
  NdkCompileOptions opts;
  opts.defines = {"DEBUG=1", "VERSION=\"1.0.0\""};
  
  EXPECT_EQ(opts.defines.size(), 2);
}

TEST(AndroidNdkCompilerTest, CompileOptionsWithIncludes) {
  NdkCompileOptions opts;
  opts.include_dirs = {"/path/to/include1", "/path/to/include2"};
  
  EXPECT_EQ(opts.include_dirs.size(), 2);
}

TEST(AndroidNdkCompilerTest, CompileOptionsOptimization) {
  NdkCompileOptions opts;
  opts.optimization_level = "-O3";
  
  EXPECT_EQ(opts.optimization_level, "-O3");
}

// Test link options building
TEST(AndroidNdkCompilerTest, LinkOptionsWithLibraries) {
  NdkLinkOptions opts;
  opts.libraries = {"log", "android"};
  
  EXPECT_EQ(opts.libraries.size(), 2);
}

TEST(AndroidNdkCompilerTest, LinkOptionsStaticLibrary) {
  NdkLinkOptions opts;
  opts.shared = false;
  
  EXPECT_FALSE(opts.shared);
}

} // namespace horcrux::core::test
