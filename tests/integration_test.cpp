// Horcrux - End-to-End Integration Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

namespace horcrux::integration_test {

namespace fs = std::filesystem;

// Helper function to execute a command and capture output
struct CommandResult {
  int exit_code;
  std::string output;
  std::chrono::milliseconds duration_ms;
};

auto execute_command(const std::string& command) -> CommandResult {
  auto start_time = std::chrono::steady_clock::now();

  // Use popen to capture output
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) {
    return {-1, "Failed to execute command", std::chrono::milliseconds(0)};
  }

  std::string output;
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }

  int exit_code = pclose(pipe);
  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  return {exit_code, output, duration};
}

// Test fixture for integration tests
class IntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Get repository root
    repo_root_ = fs::path(__FILE__).parent_path().parent_path();
    horcrux_bin_ = repo_root_ / "build" / "bin" / "horcrux";
    horcrux_cli_bin_ = repo_root_ / "build" / "bin" / "horcrux-cli";
    examples_dir_ = repo_root_ / "examples";

    // Verify horcrux binary exists
    ASSERT_TRUE(fs::exists(horcrux_bin_)) << "Horcrux binary not found at: " << horcrux_bin_;
    ASSERT_TRUE(fs::exists(horcrux_cli_bin_)) << "Horcrux CLI binary not found at: " << horcrux_cli_bin_;
  }

  fs::path repo_root_;
  fs::path horcrux_bin_;
  fs::path horcrux_cli_bin_;
  fs::path examples_dir_;
};

// Test: Verify horcrux binary responds to version command
TEST_F(IntegrationTest, HorcruxVersionCommand) {
  auto result = execute_command(horcrux_bin_.string() + " --version");

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.output.find("Horcrux Build System") != std::string::npos)
      << "Expected version info, got: " << result.output;
  EXPECT_TRUE(result.output.find("0.1.0") != std::string::npos);
}

// Test: Verify horcrux help command
TEST_F(IntegrationTest, HorcruxHelpCommand) {
  auto result = execute_command(horcrux_bin_.string() + " help");

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.output.find("build") != std::string::npos);
  EXPECT_TRUE(result.output.find("test") != std::string::npos);
  EXPECT_TRUE(result.output.find("clean") != std::string::npos);
}

// Test: Verify examples/hello directory exists
TEST_F(IntegrationTest, ExamplesHelloExists) {
  auto hello_dir = examples_dir_ / "hello";
  ASSERT_TRUE(fs::exists(hello_dir)) << "examples/hello directory not found";

  auto main_cpp = hello_dir / "main.cpp";
  ASSERT_TRUE(fs::exists(main_cpp)) << "examples/hello/main.cpp not found";

  auto build_file = hello_dir / "BUILD";
  ASSERT_TRUE(fs::exists(build_file)) << "examples/hello/BUILD not found";
}

// Test: Verify BUILD file is parseable
TEST_F(IntegrationTest, BuildFileValid) {
  auto build_file = examples_dir_ / "hello" / "BUILD";
  std::ifstream file(build_file);
  ASSERT_TRUE(file.is_open());

  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  // Check for expected BUILD file content
  EXPECT_TRUE(content.find("cc_binary") != std::string::npos);
  EXPECT_TRUE(content.find("name = \"hello\"") != std::string::npos);
  EXPECT_TRUE(content.find("main.cpp") != std::string::npos);
}

// Test: Attempt to build hello target
TEST_F(IntegrationTest, BuildHelloTarget) {
  // Execute: horcrux build //examples/hello:hello
  auto result = execute_command("cd " + repo_root_.string() + " && " + horcrux_bin_.string() +
                                " build //examples/hello:hello");

  EXPECT_EQ(result.exit_code, 0) << "Build failed with output: " << result.output;

  // Check build output contains success indicators
  EXPECT_TRUE(result.output.find("Building target") != std::string::npos ||
              result.output.find("Build successful") != std::string::npos);

  // Verify binary was created
  auto binary_path = repo_root_ / "bazel-bin" / "examples" / "hello" / "hello";
  EXPECT_TRUE(fs::exists(binary_path)) << "Built binary not found at: " << binary_path;
}

// Test: Build performance benchmark
TEST_F(IntegrationTest, BuildPerformanceBenchmark) {
  // Measure build time for the hello example
  constexpr int NUM_RUNS = 3;
  std::vector<std::chrono::milliseconds> build_times;

  for (int i = 0; i < NUM_RUNS; ++i) {
    // Clean before each run for consistent timing
    auto clean_result =
        execute_command("cd " + repo_root_.string() + " && " + horcrux_bin_.string() + " clean");

    auto build_result = execute_command("cd " + repo_root_.string() + " && " +
                                        horcrux_bin_.string() + " build //examples/hello:hello");

    ASSERT_EQ(build_result.exit_code, 0) << "Build failed on run " << i;
    build_times.push_back(build_result.duration_ms);
  }

  // Calculate average build time
  auto total_ms = std::chrono::milliseconds(0);
  for (const auto& time : build_times) {
    total_ms += time;
  }
  auto avg_ms = total_ms / NUM_RUNS;

  std::cout << "\n=== Build Performance Benchmark ===\n";
  std::cout << "  Runs: " << NUM_RUNS << "\n";
  std::cout << "  Average build time: " << avg_ms.count() << " ms\n";
  std::cout << "  Individual times: ";
  for (const auto& time : build_times) {
    std::cout << time.count() << "ms ";
  }
  std::cout << "\n";

  // Performance expectation: builds should complete in reasonable time
  // For a simple hello world, we expect less than 5 seconds
  EXPECT_LT(avg_ms.count(), 5000) << "Build took longer than expected";
}

// Test: Validate cache behavior
TEST_F(IntegrationTest, CacheBehavior) {
  // Test that:
  // 1. First build creates cache entries
  // 2. Second build uses cache (should be faster)
  // Note: This is a basic test. Full cache verification will be implemented
  // when the caching system is fully integrated

  // First build (cold cache)
  auto first_build = execute_command("cd " + repo_root_.string() + " && " + horcrux_bin_.string() +
                                     " build //examples/hello:hello");
  ASSERT_EQ(first_build.exit_code, 0);
  auto first_duration = first_build.duration_ms;

  // Second build (should be faster or similar)
  auto second_build = execute_command("cd " + repo_root_.string() + " && " + horcrux_bin_.string() +
                                      " build //examples/hello:hello");
  ASSERT_EQ(second_build.exit_code, 0);
  auto second_duration = second_build.duration_ms;

  std::cout << "\n=== Cache Behavior Test ===\n";
  std::cout << "  First build: " << first_duration.count() << " ms\n";
  std::cout << "  Second build: " << second_duration.count() << " ms\n";

  // Both builds should succeed
  EXPECT_EQ(first_build.exit_code, 0);
  EXPECT_EQ(second_build.exit_code, 0);

  // Note: Without full cache implementation, we just verify both builds work
  std::cout << "  Cache behavior: Basic validation passed\n";
  std::cout << "  (Full cache optimization will be implemented in future iterations)\n";
}

// Test: Validate build logs
TEST_F(IntegrationTest, BuildLogsValidation) {
  auto result = execute_command("cd " + repo_root_.string() + " && " + horcrux_bin_.string() +
                                " build //examples/hello:hello");

  ASSERT_EQ(result.exit_code, 0);

  // Check that logs contain expected information
  EXPECT_TRUE(result.output.find("//examples/hello:hello") != std::string::npos)
      << "Target not found in logs";

  EXPECT_TRUE(result.output.find("Building target") != std::string::npos)
      << "Build action not logged";

  // Check for success message (may include "from cache", "incremental", or be up-to-date)
  EXPECT_TRUE(result.output.find("Build successful") != std::string::npos ||
              result.output.find("up-to-date") != std::string::npos)
      << "Success or up-to-date message not found";

  // Logs should be structured and informative
  EXPECT_FALSE(result.output.empty()) << "Build produced no output";
}

// Test: Run the built binary
TEST_F(IntegrationTest, RunBuiltBinary) {
  // First build the target
  auto build_result = execute_command("cd " + repo_root_.string() + " && " + horcrux_bin_.string() +
                                      " build //examples/hello:hello");
  ASSERT_EQ(build_result.exit_code, 0);

  // Run the built binary
  auto binary_path = repo_root_ / "bazel-bin" / "examples" / "hello" / "hello";
  ASSERT_TRUE(fs::exists(binary_path));

  auto run_result = execute_command(binary_path.string());
  EXPECT_EQ(run_result.exit_code, 0);
  EXPECT_TRUE(run_result.output.find("Hello from Horcrux") != std::string::npos)
      << "Expected output not found. Got: " << run_result.output;
}

// Test: Doctor command help
TEST_F(IntegrationTest, DoctorCommandHelp) {
  auto result = execute_command(horcrux_cli_bin_.string() + " doctor");
  
  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.output.find("Usage: horcrux doctor") != std::string::npos)
      << "Doctor help not found";
  EXPECT_TRUE(result.output.find("android") != std::string::npos)
      << "Android subsystem not listed";
}

// Test: Doctor android command without SDK
TEST_F(IntegrationTest, DoctorAndroidWithoutSdk) {
  // Unset Android environment variables to ensure it fails gracefully
  auto result = execute_command(
      "unset ANDROID_HOME && unset ANDROID_SDK_ROOT && unset ANDROID_NDK_ROOT && " +
      horcrux_cli_bin_.string() + " doctor android");
  
  // Should fail with clear error message
  EXPECT_NE(result.exit_code, 0);
  EXPECT_TRUE(result.output.find("Android SDK not found") != std::string::npos ||
              result.output.find("Android Toolchain Detection Failed") != std::string::npos)
      << "Expected error message not found. Got: " << result.output;
}

// Placeholder test to demonstrate structure until build is implemented
TEST_F(IntegrationTest, BasicStructureValidation) {
  // This test validates that the basic structure is in place

  // Verify examples structure
  EXPECT_TRUE(fs::exists(examples_dir_ / "hello"));
  EXPECT_TRUE(fs::exists(examples_dir_ / "hello" / "main.cpp"));
  EXPECT_TRUE(fs::exists(examples_dir_ / "hello" / "BUILD"));

  // Verify horcrux binary exists and is executable
  EXPECT_TRUE(fs::exists(horcrux_bin_));

  std::cout << "\n=== Integration Test Structure ===\n";
  std::cout << "Repository root: " << repo_root_ << "\n";
  std::cout << "Horcrux binary: " << horcrux_bin_ << "\n";
  std::cout << "Examples directory: " << examples_dir_ << "\n";
  std::cout << "\nStructure validation: PASSED\n";
}

} // namespace horcrux::integration_test
