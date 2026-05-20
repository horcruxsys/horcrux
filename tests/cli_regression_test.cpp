// Horcrux - CLI Command Regression Tests
// Copyright (C) 2026 Horcrux Project Contributors
// Licensed under the MIT License

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/cli/clean_command.h"
#include "../src/cli/doctor_command.h"
#include "../src/cli/logger.h"
#include "../src/cli/query_command.h"
#include "../src/cli/registry_command.h"

namespace horcrux::cli::test {

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a fake argv from a list of strings
// ─────────────────────────────────────────────────────────────────────────────

class FakeArgv {
public:
  explicit FakeArgv(std::vector<std::string> args) : args_(std::move(args)) {
    for (auto& a : args_) {
      ptrs_.push_back(const_cast<char*>(a.c_str()));
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
// CleanError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(CleanCommandErrorRegressionTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(CleanError::InvalidPath), "Invalid or unsafe path");
  EXPECT_EQ(to_string(CleanError::RemovalFailed), "Failed to remove directory");
}

// ─────────────────────────────────────────────────────────────────────────────
// is_safe_path
// ─────────────────────────────────────────────────────────────────────────────

TEST(CleanCommandSafetyTest, RejectsEmptyPath) {
  EXPECT_FALSE(is_safe_path(""));
}

TEST(CleanCommandSafetyTest, RejectsRootPath) {
  EXPECT_FALSE(is_safe_path("/"));
}

TEST(CleanCommandSafetyTest, AcceptsRelativePath) {
  EXPECT_TRUE(is_safe_path("horcrux-out"));
  EXPECT_TRUE(is_safe_path(".horcrux-cache"));
  EXPECT_TRUE(is_safe_path("./build/output"));
}

TEST(CleanCommandSafetyTest, AcceptsDeepAbsolutePath) {
  EXPECT_TRUE(is_safe_path("/tmp/horcrux/test/build"));
  EXPECT_TRUE(is_safe_path("/home/user/project/output"));
  EXPECT_TRUE(is_safe_path("/var/tmp/horcrux/cache/subdir"));
}

TEST(CleanCommandSafetyTest, RejectsShallowAbsolutePath) {
  EXPECT_FALSE(is_safe_path("/tmp"));
  EXPECT_FALSE(is_safe_path("/usr"));
  EXPECT_FALSE(is_safe_path("/home"));
  EXPECT_FALSE(is_safe_path("/opt"));
  EXPECT_FALSE(is_safe_path("/var"));
}

TEST(CleanCommandSafetyTest, AcceptsThreeComponentAbsolutePath) {
  EXPECT_TRUE(is_safe_path("/tmp/a"));
  EXPECT_TRUE(is_safe_path("/usr/bin"));
}

// ─────────────────────────────────────────────────────────────────────────────
// QueryError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(QueryCommandErrorRegressionTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(QueryError::NoTargetSpecified), "No target specified");
  EXPECT_EQ(to_string(QueryError::InvalidTarget), "Invalid target specification");
  EXPECT_EQ(to_string(QueryError::TargetNotFound), "Target not found in build graph");
  EXPECT_EQ(to_string(QueryError::GraphError), "Build graph query failed");
}

// ─────────────────────────────────────────────────────────────────────────────
// handle_query_command – option parsing
// ─────────────────────────────────────────────────────────────────────────────

TEST(QueryCommandRegressionTest, NoTargetReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "query"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(QueryCommandRegressionTest, HelpFlagReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "query", "--help"});
  int rc = handle_query_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// RegistryCommandError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(RegistryCommandErrorRegressionTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(RegistryCommandError::NoSubcommand), "No registry subcommand provided");
  EXPECT_EQ(to_string(RegistryCommandError::InvalidSubcommand), "Invalid registry subcommand");
  EXPECT_EQ(to_string(RegistryCommandError::MissingArgument), "Missing required argument");
  EXPECT_EQ(to_string(RegistryCommandError::OperationFailed), "Registry operation failed");
}

// ─────────────────────────────────────────────────────────────────────────────
// handle_registry_command – option parsing
// ─────────────────────────────────────────────────────────────────────────────

TEST(RegistryCommandRegressionTest, NoSubcommandReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "registry"});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(RegistryCommandRegressionTest, HelpFlagReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "registry", "--help"});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(RegistryCommandRegressionTest, UnknownSubcommandReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "registry", "unknown-sub"});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(RegistryCommandRegressionTest, AddWithTooFewArgsReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "registry", "add"});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(RegistryCommandRegressionTest, RemoveWithTooFewArgsReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "registry", "remove"});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// handle_doctor_command – option parsing
// ─────────────────────────────────────────────────────────────────────────────

TEST(DoctorCommandRegressionTest, NoSubsystemPrintsUsage) {
  Logger logger;
  FakeArgv args({"horcrux", "doctor"});
  // No subsystem: prints usage and returns 0
  int rc = handle_doctor_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(DoctorCommandRegressionTest, UnknownSubsystemReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "doctor", "unknown"});
  int rc = handle_doctor_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(DoctorCommandRegressionTest, SystemNotYetImplementedReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "doctor", "system"});
  int rc = handle_doctor_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(DoctorCommandRegressionTest, CacheNotYetImplementedReturnsError) {
  Logger logger;
  FakeArgv args({"horcrux", "doctor", "cache"});
  int rc = handle_doctor_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(DoctorCommandRegressionTest, AndroidReturnsNonZeroWithoutSdk) {
  Logger logger;
  FakeArgv args({"horcrux", "doctor", "android"});
  // Without a real Android SDK on the test machine, this should fail gracefully
  int rc = handle_doctor_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

} // namespace horcrux::cli::test
