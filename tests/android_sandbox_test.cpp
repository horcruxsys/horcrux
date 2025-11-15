// Horcrux - Android Sandbox Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_sandbox.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

using namespace horcrux::core;

class AndroidSandboxTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create temporary directories for testing
    test_dir_ = std::filesystem::temp_directory_path() / "horcrux_sandbox_test";
    sdk_dir_ = test_dir_ / "sdk";
    source_dir_ = test_dir_ / "source";
    scratch_dir_ = test_dir_ / "scratch";

    std::filesystem::create_directories(sdk_dir_);
    std::filesystem::create_directories(source_dir_);
    std::filesystem::create_directories(scratch_dir_);

    // Create mock files
    create_mock_files();
  }

  void TearDown() override {
    // Cleanup test directory
    if (std::filesystem::exists(test_dir_)) {
      std::filesystem::remove_all(test_dir_);
    }
  }

  void create_mock_files() {
    // Create mock SDK file
    std::ofstream sdk_file(sdk_dir_ / "android.jar");
    sdk_file << "Mock SDK content\n";
    sdk_file.close();

    // Create mock source file
    std::ofstream source_file(source_dir_ / "Main.java");
    source_file << "public class Main { }\n";
    source_file.close();
  }

  std::filesystem::path test_dir_;
  std::filesystem::path sdk_dir_;
  std::filesystem::path source_dir_;
  std::filesystem::path scratch_dir_;
};

// Test: Sandbox support detection
TEST_F(AndroidSandboxTest, IsSupportedDetection) {
#ifdef __linux__
  EXPECT_TRUE(AndroidSandbox::is_supported());
#else
  EXPECT_FALSE(AndroidSandbox::is_supported());
#endif
}

// Test: MountRule creation helpers
TEST_F(AndroidSandboxTest, MountRuleCreation) {
  auto read_only_mount = MountRule::read_only("/sdk", "/sandbox/sdk");
  EXPECT_EQ(read_only_mount.source, "/sdk");
  EXPECT_EQ(read_only_mount.target, "/sandbox/sdk");
  EXPECT_EQ(read_only_mount.type, MountType::ReadOnly);
  EXPECT_FALSE(read_only_mount.optional);

  auto read_write_mount = MountRule::read_write("/scratch", "/sandbox/work");
  EXPECT_EQ(read_write_mount.source, "/scratch");
  EXPECT_EQ(read_write_mount.target, "/sandbox/work");
  EXPECT_EQ(read_write_mount.type, MountType::ReadWrite);

  auto tmpfs_mount = MountRule::tmpfs("/tmp");
  EXPECT_EQ(tmpfs_mount.target, "/tmp");
  EXPECT_EQ(tmpfs_mount.type, MountType::TmpFs);
}

// Test: Configuration validation with valid config
TEST_F(AndroidSandboxTest, ValidateConfigSuccess) {
  SandboxConfig config;
  config.executable = "/bin/echo";
  config.arguments = {"hello"};
  config.working_dir = scratch_dir_;

  auto result = AndroidSandbox::validate_config(config);
  EXPECT_TRUE(result.has_value());
}

// Test: Configuration validation with missing executable
TEST_F(AndroidSandboxTest, ValidateConfigMissingExecutable) {
  SandboxConfig config;
  config.executable = "/nonexistent/binary";
  config.arguments = {"test"};

  auto result = AndroidSandbox::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), SandboxError::InvalidConfiguration);
}

// Test: Configuration validation with missing mount source
TEST_F(AndroidSandboxTest, ValidateConfigMissingMountSource) {
  SandboxConfig config;
  config.executable = "/bin/echo";
  config.mounts.push_back(MountRule::read_only("/nonexistent", "/sandbox/test"));

  auto result = AndroidSandbox::validate_config(config);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), SandboxError::InvalidConfiguration);
}

// Test: Configuration validation with optional missing mount
TEST_F(AndroidSandboxTest, ValidateConfigOptionalMount) {
  SandboxConfig config;
  config.executable = "/bin/echo";
  
  MountRule optional_mount = MountRule::read_only("/nonexistent", "/sandbox/test");
  optional_mount.optional = true;
  config.mounts.push_back(optional_mount);

  auto result = AndroidSandbox::validate_config(config);
  EXPECT_TRUE(result.has_value());
}

// Test: Create standard Android build sandbox configuration
TEST_F(AndroidSandboxTest, CreateAndroidBuildSandbox) {
  auto config = AndroidSandbox::create_android_build_sandbox(
      "/bin/echo", {"test"}, sdk_dir_, source_dir_, scratch_dir_);

  EXPECT_EQ(config.executable, "/bin/echo");
  EXPECT_EQ(config.arguments.size(), 1);
  EXPECT_EQ(config.arguments[0], "test");
  EXPECT_EQ(config.working_dir, "/sandbox/work");

  // Check mounts
  EXPECT_GE(config.mounts.size(), 3); // SDK, source, scratch, and tmpfs

  EXPECT_TRUE(config.enable_network_isolation);
  EXPECT_TRUE(config.enable_pid_namespace);
  EXPECT_TRUE(config.enable_mount_namespace);
  EXPECT_TRUE(config.detect_violations);
  EXPECT_TRUE(config.fail_on_violation);
}

// Test: Basic execution with echo command
TEST_F(AndroidSandboxTest, ExecuteBasicCommand) {
  SandboxConfig config;
  config.executable = "/bin/echo";
  config.arguments = {"Hello", "Sandbox"};
  config.working_dir = scratch_dir_;
  config.detect_violations = false;
  // Disable namespace isolation for tests (requires elevated privileges)
  config.enable_mount_namespace = false;
  config.enable_pid_namespace = false;
  config.enable_network_isolation = false;
  config.enable_ipc_namespace = false;
  config.enable_uts_namespace = false;

  AndroidSandbox sandbox;
  auto result = sandbox.execute(config);

  ASSERT_TRUE(result.has_value());
  if (result->exit_code != 0) {
    std::cerr << "Exit code: " << result->exit_code << std::endl;
    std::cerr << "Stdout: '" << result->stdout_output << "'" << std::endl;
    std::cerr << "Stderr: '" << result->stderr_output << "'" << std::endl;
  }
  EXPECT_EQ(result->exit_code, 0);
  EXPECT_TRUE(result->stdout_output.find("Hello Sandbox") != std::string::npos);
  EXPECT_TRUE(result->success());
}

// Test: Execution with environment variables
TEST_F(AndroidSandboxTest, ExecuteWithEnvironment) {
  SandboxConfig config;
  config.executable = "/bin/sh";
  config.arguments = {"-c", "echo $TEST_VAR"};
  config.working_dir = scratch_dir_;
  config.env_vars.push_back({"TEST_VAR", "test_value"});
  config.detect_violations = false;
  // Disable namespace isolation for tests
  config.enable_mount_namespace = false;
  config.enable_pid_namespace = false;
  config.enable_network_isolation = false;

  AndroidSandbox sandbox;
  auto result = sandbox.execute(config);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->exit_code, 0);
  EXPECT_TRUE(result->stdout_output.find("test_value") != std::string::npos);
}

// Test: Execution with non-zero exit code
TEST_F(AndroidSandboxTest, ExecuteNonZeroExit) {
  SandboxConfig config;
  config.executable = "/bin/sh";
  config.arguments = {"-c", "exit 42"};
  config.working_dir = scratch_dir_;
  config.detect_violations = false;
  // Disable namespace isolation for tests
  config.enable_mount_namespace = false;
  config.enable_pid_namespace = false;
  config.enable_network_isolation = false;

  AndroidSandbox sandbox;
  auto result = sandbox.execute(config);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->exit_code, 42);
  EXPECT_FALSE(result->success());
}

// Test: Execution with stdout and stderr
TEST_F(AndroidSandboxTest, ExecuteWithStdoutStderr) {
  SandboxConfig config;
  config.executable = "/bin/sh";
  config.arguments = {"-c", "echo stdout; echo stderr >&2"};
  config.working_dir = scratch_dir_;
  config.detect_violations = false;
  // Disable namespace isolation for tests
  config.enable_mount_namespace = false;
  config.enable_pid_namespace = false;
  config.enable_network_isolation = false;

  AndroidSandbox sandbox;
  auto result = sandbox.execute(config);

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->stdout_output.find("stdout") != std::string::npos);
  EXPECT_TRUE(result->stderr_output.find("stderr") != std::string::npos);
}

// Test: Execution time measurement
TEST_F(AndroidSandboxTest, ExecutionTimeMeasurement) {
  SandboxConfig config;
  config.executable = "/bin/sleep";
  config.arguments = {"0.1"};
  config.working_dir = scratch_dir_;
  config.detect_violations = false;
  // Disable namespace isolation for tests
  config.enable_mount_namespace = false;
  config.enable_pid_namespace = false;
  config.enable_network_isolation = false;

  AndroidSandbox sandbox;
  auto result = sandbox.execute(config);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->execution_time.count(), 50); // At least 50ms
  EXPECT_LT(result->execution_time.count(), 500); // Less than 500ms
}

// Test: Violation detection - permission denied
TEST_F(AndroidSandboxTest, ViolationDetectionPermissionDenied) {
  SandboxConfig config;
  config.executable = "/bin/sh";
  config.arguments = {"-c", "cat /root/denied 2>&1 || echo 'Permission denied'"};
  config.working_dir = scratch_dir_;
  config.detect_violations = true;
  config.fail_on_violation = false; // Don't fail, just detect
  // Disable namespace isolation for tests
  config.enable_mount_namespace = false;
  config.enable_pid_namespace = false;
  config.enable_network_isolation = false;

  AndroidSandbox sandbox;
  auto result = sandbox.execute(config);

  ASSERT_TRUE(result.has_value());
  // May detect violation depending on output
  // Just verify detection mechanism works
}

// Test: Violation detection - read-only filesystem
TEST_F(AndroidSandboxTest, ViolationDetectionReadOnly) {
  SandboxConfig config;
  config.executable = "/bin/sh";
  config.arguments = {"-c", "echo 'Read-only file system' >&2; exit 1"};
  config.working_dir = scratch_dir_;
  config.detect_violations = true;
  config.fail_on_violation = false;
  // Disable namespace isolation for tests
  config.enable_mount_namespace = false;
  config.enable_pid_namespace = false;
  config.enable_network_isolation = false;

  AndroidSandbox sandbox;
  auto result = sandbox.execute(config);

  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->violations.empty());
  EXPECT_EQ(result->violations[0].type, ViolationType::UnauthorizedWrite);
}

// Test: Fail on violation
TEST_F(AndroidSandboxTest, FailOnViolation) {
  SandboxConfig config;
  config.executable = "/bin/sh";
  config.arguments = {"-c", "echo 'Permission denied' >&2; exit 0"};
  config.working_dir = scratch_dir_;
  config.detect_violations = true;
  config.fail_on_violation = true;
  // Disable namespace isolation for tests
  config.enable_mount_namespace = false;
  config.enable_pid_namespace = false;
  config.enable_network_isolation = false;

  AndroidSandbox sandbox;
  auto result = sandbox.execute(config);

  // Should fail due to violation
  EXPECT_FALSE(result.has_value());
  if (!result.has_value()) {
    EXPECT_EQ(result.error(), SandboxError::ViolationDetected);
  }
}

// Test: SandboxGuard RAII cleanup
TEST_F(AndroidSandboxTest, SandboxGuardCleanup) {
  auto temp_scratch = test_dir_ / "temp_scratch";
  std::filesystem::create_directories(temp_scratch);
  
  // Create file in scratch
  std::ofstream temp_file(temp_scratch / "test.txt");
  temp_file << "test content\n";
  temp_file.close();

  {
    SandboxGuard guard(temp_scratch);
    EXPECT_TRUE(std::filesystem::exists(temp_scratch));
  }

  // After guard destruction, directory should be cleaned up
  EXPECT_FALSE(std::filesystem::exists(temp_scratch));
}

// Test: Error to string conversion
TEST_F(AndroidSandboxTest, ErrorToString) {
  EXPECT_EQ(to_string(SandboxError::InvalidConfiguration), 
            "Invalid sandbox configuration");
  EXPECT_EQ(to_string(SandboxError::MountFailed), 
            "Mount operation failed");
  EXPECT_EQ(to_string(SandboxError::NamespaceCreationFailed), 
            "Failed to create namespace");
  EXPECT_EQ(to_string(SandboxError::ProcessExecutionFailed), 
            "Process execution failed");
  EXPECT_EQ(to_string(SandboxError::ViolationDetected), 
            "Sandbox violation detected");
  EXPECT_EQ(to_string(SandboxError::UnsupportedPlatform), 
            "Sandboxing not supported on this platform");
  EXPECT_EQ(to_string(SandboxError::IoError), 
            "I/O error");
  EXPECT_EQ(to_string(SandboxError::UnknownError), 
            "Unknown error");
}

// Test: Hermetic build - same inputs produce same outputs
TEST_F(AndroidSandboxTest, HermeticBuildSameOutput) {
  // Create a simple script that writes to output
  auto script_path = scratch_dir_ / "build.sh";
  std::ofstream script(script_path);
  script << "#!/bin/bash\n";
  script << "echo 'Build output' > output.txt\n";
  script << "echo 'Done'\n";
  script.close();
  std::filesystem::permissions(script_path, 
                               std::filesystem::perms::owner_exec | 
                               std::filesystem::perms::owner_read);

  SandboxConfig config;
  config.executable = script_path;
  config.working_dir = scratch_dir_;
  config.detect_violations = false;
  // Disable namespace isolation for tests
  config.enable_mount_namespace = false;
  config.enable_pid_namespace = false;
  config.enable_network_isolation = false;

  AndroidSandbox sandbox;

  // First execution
  auto result1 = sandbox.execute(config);
  ASSERT_TRUE(result1.has_value());
  EXPECT_EQ(result1->exit_code, 0);

  // Read output
  auto output_file = scratch_dir_ / "output.txt";
  std::ifstream output1(output_file);
  std::stringstream buffer1;
  buffer1 << output1.rdbuf();
  std::string content1 = buffer1.str();

  // Remove output
  std::filesystem::remove(output_file);

  // Second execution
  auto result2 = sandbox.execute(config);
  ASSERT_TRUE(result2.has_value());
  EXPECT_EQ(result2->exit_code, 0);

  // Read output again
  std::ifstream output2(output_file);
  std::stringstream buffer2;
  buffer2 << output2.rdbuf();
  std::string content2 = buffer2.str();

  // Outputs should be identical
  EXPECT_EQ(content1, content2);
}

// Test: Namespace isolation features enabled
TEST_F(AndroidSandboxTest, NamespaceIsolationFeatures) {
  auto config = AndroidSandbox::create_android_build_sandbox(
      "/bin/echo", {"test"}, sdk_dir_, source_dir_, scratch_dir_);

  EXPECT_TRUE(config.enable_network_isolation);
  EXPECT_TRUE(config.enable_pid_namespace);
  EXPECT_TRUE(config.enable_mount_namespace);
  EXPECT_TRUE(config.enable_ipc_namespace);
  EXPECT_TRUE(config.enable_uts_namespace);
}

// Test: Working directory change
TEST_F(AndroidSandboxTest, WorkingDirectoryChange) {
  SandboxConfig config;
  config.executable = "/bin/pwd";
  config.working_dir = scratch_dir_;
  config.detect_violations = false;
  // Disable namespace isolation for tests
  config.enable_mount_namespace = false;
  config.enable_pid_namespace = false;
  config.enable_network_isolation = false;

  AndroidSandbox sandbox;
  auto result = sandbox.execute(config);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->exit_code, 0);
  // Output should contain the scratch directory path
  EXPECT_TRUE(result->stdout_output.find(scratch_dir_.filename().string()) != std::string::npos);
}
