// Horcrux - Target Parser Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <gtest/gtest.h>

#include "../src/cli/target_parser.h"

namespace horcrux::cli::test {

// Test successful parsing
TEST(TargetParserTest, ParseFullTargetSpec) {
  auto result = parse_target("//examples/hello:app");
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->package, "examples/hello");
  EXPECT_EQ(result->target_name, "app");
  EXPECT_EQ(result->label(), "//examples/hello:app");
}

TEST(TargetParserTest, ParsePackageWithImplicitTarget) {
  auto result = parse_target("//examples/hello");
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->package, "examples/hello");
  EXPECT_EQ(result->target_name, "hello");
  EXPECT_EQ(result->label(), "//examples/hello:hello");
}

TEST(TargetParserTest, ParseSingleLevelPackage) {
  auto result = parse_target("//core:lib");
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->package, "core");
  EXPECT_EQ(result->target_name, "lib");
  EXPECT_EQ(result->label(), "//core:lib");
}

// Test error cases
TEST(TargetParserTest, RejectEmptyTarget) {
  auto result = parse_target("");
  
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), TargetParseError::EmptyTarget);
}

TEST(TargetParserTest, RejectInvalidFormat) {
  auto result = parse_target("examples/hello:app");  // Missing "//"
  
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), TargetParseError::InvalidFormat);
}

TEST(TargetParserTest, RejectEmptyTargetName) {
  auto result = parse_target("//examples/hello:");
  
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), TargetParseError::MissingTargetName);
}

TEST(TargetParserTest, RejectEmptyPackage) {
  auto result = parse_target("//:target");
  
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), TargetParseError::MissingPackage);
}

// Test edge cases
TEST(TargetParserTest, ParseDeepPackagePath) {
  auto result = parse_target("//src/main/cpp/core:library");
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->package, "src/main/cpp/core");
  EXPECT_EQ(result->target_name, "library");
}

TEST(TargetParserTest, ParseTargetWithNumbers) {
  auto result = parse_target("//app2:lib3");
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->package, "app2");
  EXPECT_EQ(result->target_name, "lib3");
}

// Test error message conversion
TEST(TargetParserTest, ErrorToString) {
  EXPECT_EQ(to_string(TargetParseError::InvalidFormat), "Invalid target format");
  EXPECT_EQ(to_string(TargetParseError::EmptyTarget), "Empty target specification");
  EXPECT_EQ(to_string(TargetParseError::MissingPackage), "Missing package in target specification");
  EXPECT_EQ(to_string(TargetParseError::MissingTargetName), "Missing target name in specification");
}

} // namespace horcrux::cli::test
