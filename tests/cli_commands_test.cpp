// Horcrux - CLI Command Unit Tests (test, clean, query)
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/cli/build_executor.h"
#include "../src/cli/clean_command.h"
#include "../src/cli/logger.h"
#include "../src/cli/query_command.h"
#include "../src/cli/test_command.h"

namespace horcrux::cli::test {

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a fake argv from a list of strings
// ─────────────────────────────────────────────────────────────────────────────

class FakeArgv {
public:
  explicit FakeArgv(std::vector<std::string> args) : args_(std::move(args)) {
    for (auto& a : args_) {
      ptrs_.push_back(
          const_cast<char*>(a.c_str())); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }
  }

  auto argc() const -> int {
    return static_cast<int>(ptrs_.size());
  }
  auto argv() -> char** {
    return ptrs_.data();
  }

private:
  std::vector<std::string> args_;
  std::vector<char*> ptrs_;
};

// ─────────────────────────────────────────────────────────────────────────────
// TestError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(TestCommandErrorTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(TestError::NoTargetsSpecified), "No test targets specified");
  EXPECT_EQ(to_string(TestError::InvalidTarget), "Invalid target specification");
  EXPECT_EQ(to_string(TestError::BuildFailed), "Build step failed before test execution");
  EXPECT_EQ(to_string(TestError::ExecutionFailed), "Test execution failed");
}

// ─────────────────────────────────────────────────────────────────────────────
// handle_test_command – option parsing
// ─────────────────────────────────────────────────────────────────────────────

TEST(TestCommandTest, NoTargetsReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "test"});
  int rc = handle_test_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(TestCommandTest, HelpFlagReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "test", "--help"});
  int rc = handle_test_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(TestCommandTest, InvalidTargetIsCountedAsFailed) {
  Logger logger;
  FakeArgv args({"horcrux", "test", "not-a-valid-target"});
  int rc = handle_test_command(args.argc(), args.argv(), logger);
  // Must be non-zero because the target is invalid
  EXPECT_NE(rc, 0);
}

TEST(TestCommandTest, ValidTargetReturnsZeroOrNonzero) {
  Logger logger;
  FakeArgv args({"horcrux", "test", "//examples/hello:app"});
  int rc = handle_test_command(args.argc(), args.argv(), logger);
  // Valid target in the demo graph – build succeeds, test passes
  EXPECT_EQ(rc, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// CleanError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(CleanCommandErrorTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(CleanError::InvalidPath), "Invalid or unsafe path");
  EXPECT_EQ(to_string(CleanError::RemovalFailed), "Failed to remove directory");
}

// ─────────────────────────────────────────────────────────────────────────────
// handle_clean_command – option parsing
// ─────────────────────────────────────────────────────────────────────────────

TEST(CleanCommandTest, HelpFlagReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "clean", "--help"});
  int rc = handle_clean_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(CleanCommandTest, DryRunReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "clean", "--dry-run", "--output-dir=/tmp/horcrux_test_clean_out",
                 "--cache-dir=/tmp/horcrux_test_clean_cache"});
  int rc = handle_clean_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(CleanCommandTest, DryRunWithScopeOutputsReturnsZero) {
  Logger logger;
  FakeArgv args(
      {"horcrux", "clean", "--outputs", "--dry-run", "--output-dir=/tmp/horcrux_test_clean_out2"});
  int rc = handle_clean_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(CleanCommandTest, DryRunWithScopeCacheReturnsZero) {
  Logger logger;
  FakeArgv args(
      {"horcrux", "clean", "--cache", "--dry-run", "--cache-dir=/tmp/horcrux_test_clean_cache2"});
  int rc = handle_clean_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(CleanCommandTest, DefaultScopeIsAll) {
  // Verify both default output_dir and cache_dir are handled in dry-run
  Logger logger;
  FakeArgv args({"horcrux", "clean", "--all", "--dry-run",
                 "--output-dir=/tmp/horcrux_test_clean_all_out",
                 "--cache-dir=/tmp/horcrux_test_clean_all_cache"});
  int rc = handle_clean_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(CleanCommandTest, RemovesExistingDirectory) {
  namespace fs = std::filesystem;

  // Create a temporary directory to be cleaned
  auto tmp_out = fs::temp_directory_path() / "horcrux_clean_test_out";
  auto tmp_cache = fs::temp_directory_path() / "horcrux_clean_test_cache";
  fs::create_directories(tmp_out);
  fs::create_directories(tmp_cache);

  Logger logger;
  FakeArgv args({"horcrux", "clean", "--all", "--output-dir=" + tmp_out.string(),
                 "--cache-dir=" + tmp_cache.string()});
  int rc = handle_clean_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
  EXPECT_FALSE(fs::exists(tmp_out));
  EXPECT_FALSE(fs::exists(tmp_cache));
}

// ─────────────────────────────────────────────────────────────────────────────
// QueryError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(QueryCommandErrorTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(QueryError::NoTargetSpecified), "No target specified");
  EXPECT_EQ(to_string(QueryError::InvalidTarget), "Invalid target specification");
  EXPECT_EQ(to_string(QueryError::TargetNotFound), "Target not found in build graph");
  EXPECT_EQ(to_string(QueryError::GraphError), "Build graph query failed");
}

// ─────────────────────────────────────────────────────────────────────────────
// handle_query_command – option parsing and correctness
// ─────────────────────────────────────────────────────────────────────────────

TEST(QueryCommandTest, HelpFlagReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--help"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, NoTargetWithDepsQueryReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--deps"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(QueryCommandTest, AllTargetsQueryReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--all"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, AllTargetsJsonOutputReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--all", "--output=json"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, DirectDepsForKnownTargetReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--deps", "//examples/hello:app"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, TransitiveDepsForKnownTargetReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--trans-deps", "//examples/hello:app"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, ReverseDepsForKnownTargetReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--rdeps", "//examples/hello:lib"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, TopoOrderReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--topo", "//examples/hello:app"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, UnknownTargetReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--deps", "//does/not:exist"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Android example targets are visible via query
// ─────────────────────────────────────────────────────────────────────────────

TEST(QueryCommandTest, AndroidTargetDirectDepsReturnsZero) {
  // ndk-app:app depends on ndk-app:native_lib (a cc_library)
  Logger logger;
  FakeArgv args({"horcrux", "query", "--deps", "//examples/android/ndk-app:app"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, AndroidTargetTransitiveDepsReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--trans-deps", "//examples/android/ndk-app:app"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, AndroidNativeLibReverseDepsReturnsZero) {
  // native_lib is depended upon by ndk-app:app
  Logger logger;
  FakeArgv args({"horcrux", "query", "--rdeps", "//examples/android/ndk-app:native_lib"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, BasicXmlAndroidAppQueryReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--deps", "//examples/android/basic-xml-app:app"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(QueryCommandTest, BuildExecutorWorkspaceGraphIncludesAndroidTargets) {
  // Verify the workspace graph exposed by BuildExecutor includes Android targets
  auto graph_result = BuildExecutor::create_workspace_graph();
  ASSERT_TRUE(graph_result.has_value());

  const auto& graph = *graph_result;
  EXPECT_NE(graph.get_node("//examples/android/basic-xml-app:app"), nullptr);
  EXPECT_NE(graph.get_node("//examples/android/ndk-app:app"), nullptr);
  EXPECT_NE(graph.get_node("//examples/android/ndk-app:native_lib"), nullptr);
  EXPECT_NE(graph.get_node("//examples/android/compose-app:app"), nullptr);
  EXPECT_NE(graph.get_node("//examples/android/multi-flavor-app:app"), nullptr);
  EXPECT_NE(graph.get_node("//examples/android/multi-module-app:app"), nullptr);

  // ndk-app:app must depend on ndk-app:native_lib
  auto deps = graph.get_dependencies("//examples/android/ndk-app:app");
  ASSERT_TRUE(deps.has_value());
  EXPECT_EQ(deps->size(), 1u);
  EXPECT_EQ((*deps)[0], "//examples/android/ndk-app:native_lib");
}

} // namespace horcrux::cli::test
