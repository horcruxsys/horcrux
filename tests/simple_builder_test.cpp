// Horcrux - SimpleBuilder Unit Tests
// Copyright (C) 2026 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/simple_builder.h"

namespace horcrux::core::test {

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// BuildError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(SimpleBuilderErrorTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(BuildError::InvalidTarget), "Invalid target format");
  EXPECT_EQ(to_string(BuildError::CompilationFailed), "Compilation failed");
  EXPECT_EQ(to_string(BuildError::SourceNotFound), "Source file not found");
  EXPECT_EQ(to_string(BuildError::BuildFileNotFound), "BUILD file not found");
  EXPECT_EQ(to_string(BuildError::CacheError), "Cache operation failed");
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_target
// ─────────────────────────────────────────────────────────────────────────────

TEST(SimpleBuilderParseTargetTest, ValidTarget) {
  SimpleBuilder builder;
  auto result = builder.parse_target_for_test("//examples/hello:app");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->package_path, "examples/hello");
  EXPECT_EQ(result->target_name, "app");
}

TEST(SimpleBuilderParseTargetTest, ValidTargetNestedPath) {
  SimpleBuilder builder;
  auto result = builder.parse_target_for_test("//src/core/main:run");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->package_path, "src/core/main");
  EXPECT_EQ(result->target_name, "run");
}

TEST(SimpleBuilderParseTargetTest, InvalidTargetNoDoubleSlash) {
  SimpleBuilder builder;
  auto result = builder.parse_target_for_test("examples/hello:app");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), BuildError::InvalidTarget);
}

TEST(SimpleBuilderParseTargetTest, InvalidTargetNoColon) {
  SimpleBuilder builder;
  auto result = builder.parse_target_for_test("//examples/hello");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), BuildError::InvalidTarget);
}

TEST(SimpleBuilderParseTargetTest, InvalidTargetEmpty) {
  SimpleBuilder builder;
  auto result = builder.parse_target_for_test("");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), BuildError::InvalidTarget);
}

TEST(SimpleBuilderParseTargetTest, InvalidTargetJustColon) {
  SimpleBuilder builder;
  auto result = builder.parse_target_for_test("//:");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), BuildError::InvalidTarget);
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_all_targets
// ─────────────────────────────────────────────────────────────────────────────

class SimpleBuilderParseTest : public ::testing::Test {
protected:
  void SetUp() override {
    dir_ = fs::temp_directory_path() / "horcrux_sb_parse_test";
    fs::remove_all(dir_);
    fs::create_directories(dir_);
  }

  void TearDown() override {
    fs::remove_all(dir_);
  }

  void write_build(const std::string& content) {
    std::ofstream f(dir_ / "BUILD");
    f << content;
  }

  fs::path dir_;
};

TEST_F(SimpleBuilderParseTest, ParseCcBinary) {
  write_build(R"(cc_binary(
    name = "hello",
    srcs = ["main.cpp", "lib.cpp"],
    deps = [":greet"],
))");
  SimpleBuilder builder;
  auto targets = builder.parse_all_targets_for_test(dir_ / "BUILD");
  ASSERT_EQ(targets.size(), 1u);
  EXPECT_EQ(targets[0].rule_type, "cc_binary");
  EXPECT_EQ(targets[0].name, "hello");
  ASSERT_EQ(targets[0].srcs.size(), 2u);
  EXPECT_EQ(targets[0].srcs[0], "main.cpp");
  EXPECT_EQ(targets[0].srcs[1], "lib.cpp");
  ASSERT_EQ(targets[0].deps.size(), 1u);
  EXPECT_EQ(targets[0].deps[0], ":greet");
}

TEST_F(SimpleBuilderParseTest, ParseJavaBinary) {
  write_build(R"(java_binary(
    name = "app",
    srcs = ["Main.java"],
    deps = [":lib"],
    main_class = "com.example.Main",
))");
  SimpleBuilder builder;
  auto targets = builder.parse_all_targets_for_test(dir_ / "BUILD");
  ASSERT_EQ(targets.size(), 1u);
  EXPECT_EQ(targets[0].rule_type, "java_binary");
  EXPECT_EQ(targets[0].main_class, "com.example.Main");
}

TEST_F(SimpleBuilderParseTest, ParsePyBinary) {
  write_build(R"(py_binary(
    name = "my_script",
    srcs = ["main.py"],
    main = "main.py",
))");
  SimpleBuilder builder;
  auto targets = builder.parse_all_targets_for_test(dir_ / "BUILD");
  ASSERT_EQ(targets.size(), 1u);
  EXPECT_EQ(targets[0].rule_type, "py_binary");
  EXPECT_EQ(targets[0].main_file, "main.py");
}

TEST_F(SimpleBuilderParseTest, ParseRustBinary) {
  write_build(R"(rust_binary(
    name = "greeter",
    srcs = ["main.rs"],
    edition = "2021",
))");
  SimpleBuilder builder;
  auto targets = builder.parse_all_targets_for_test(dir_ / "BUILD");
  ASSERT_EQ(targets.size(), 1u);
  EXPECT_EQ(targets[0].rule_type, "rust_binary");
  EXPECT_EQ(targets[0].edition, "2021");
}

TEST_F(SimpleBuilderParseTest, ParseMultipleTargets) {
  write_build(R"(cc_library(
    name = "lib",
    srcs = ["lib.cpp"],
)
cc_binary(
    name = "app",
    srcs = ["main.cpp"],
    deps = [":lib"],
))");
  SimpleBuilder builder;
  auto targets = builder.parse_all_targets_for_test(dir_ / "BUILD");
  ASSERT_EQ(targets.size(), 2u);
  EXPECT_EQ(targets[0].name, "lib");
  EXPECT_EQ(targets[0].rule_type, "cc_library");
  EXPECT_EQ(targets[1].name, "app");
  EXPECT_EQ(targets[1].rule_type, "cc_binary");
}

TEST_F(SimpleBuilderParseTest, NoTargetsWhenNameMissing) {
  write_build(R"(cc_binary(
    srcs = ["main.cpp"],
))");
  SimpleBuilder builder;
  auto targets = builder.parse_all_targets_for_test(dir_ / "BUILD");
  EXPECT_TRUE(targets.empty());
}

TEST_F(SimpleBuilderParseTest, MissingBuildFileReturnsEmpty) {
  SimpleBuilder builder;
  auto targets = builder.parse_all_targets_for_test(dir_ / "NONEXISTENT");
  EXPECT_TRUE(targets.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// collect_all_sources
// ─────────────────────────────────────────────────────────────────────────────

TEST(SimpleBuilderCollectSourcesTest, DirectSources) {
  ParsedTarget target;
  target.name = "app";
  target.rule_type = "cc_binary";
  target.srcs = {"main.cpp"};

  std::vector<ParsedTarget> all_targets = {target};
  fs::path package_dir = "/pkg";

  SimpleBuilder builder;
  auto sources = builder.collect_all_sources_for_test(target, all_targets, package_dir);
  ASSERT_EQ(sources.size(), 1u);
  EXPECT_EQ(sources[0], fs::path("/pkg/main.cpp"));
}

TEST(SimpleBuilderCollectSourcesTest, RecursiveDeps) {
  ParsedTarget lib;
  lib.name = "lib";
  lib.srcs = {"lib.cpp"};

  ParsedTarget app;
  app.name = "app";
  app.srcs = {"main.cpp"};
  app.deps = {":lib"};

  std::vector<ParsedTarget> all_targets = {lib, app};
  fs::path package_dir = "/pkg";

  SimpleBuilder builder;
  auto sources = builder.collect_all_sources_for_test(app, all_targets, package_dir);
  ASSERT_EQ(sources.size(), 2u);
  EXPECT_EQ(sources[0], fs::path("/pkg/lib.cpp"));  // dep first
  EXPECT_EQ(sources[1], fs::path("/pkg/main.cpp")); // then own srcs
}

TEST(SimpleBuilderCollectSourcesTest, DeduplicatesDeps) {
  ParsedTarget lib;
  lib.name = "shared_lib";
  lib.srcs = {"shared.cpp"};

  ParsedTarget app;
  app.name = "app";
  app.srcs = {"main.cpp"};
  app.deps = {":shared_lib", ":shared_lib"};

  std::vector<ParsedTarget> all_targets = {lib, app};

  SimpleBuilder builder;
  auto sources = builder.collect_all_sources_for_test(app, all_targets, "/pkg");
  // shared_lib should appear only once despite being listed twice
  EXPECT_EQ(sources.size(), 2u);
}

} // namespace horcrux::core::test
