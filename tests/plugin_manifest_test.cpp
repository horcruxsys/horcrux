// Horcrux - Plugin Manifest Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <gtest/gtest.h>

#include "../src/core/plugin_manifest.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// PluginVersion
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginVersionTest, ParseValid) {
  auto v = PluginVersion::parse("1.2.3");
  ASSERT_TRUE(v.has_value());
  EXPECT_EQ(v->ver_major, 1u);
  EXPECT_EQ(v->ver_minor, 2u);
  EXPECT_EQ(v->ver_patch, 3u);
}

TEST(PluginVersionTest, ParseZero) {
  auto v = PluginVersion::parse("0.0.0");
  ASSERT_TRUE(v.has_value());
  EXPECT_EQ(v->ver_major, 0u);
  EXPECT_EQ(v->ver_minor, 0u);
  EXPECT_EQ(v->ver_patch, 0u);
}

TEST(PluginVersionTest, ParseMissingDot) {
  auto v = PluginVersion::parse("1.2");
  EXPECT_FALSE(v.has_value());
}

TEST(PluginVersionTest, ParseNonNumeric) {
  auto v = PluginVersion::parse("1.a.3");
  EXPECT_FALSE(v.has_value());
}

TEST(PluginVersionTest, ToString) {
  PluginVersion v{1, 2, 3};
  EXPECT_EQ(v.to_string(), "1.2.3");
}

TEST(PluginVersionTest, Comparison) {
  PluginVersion v100{1, 0, 0};
  PluginVersion v110{1, 1, 0};
  PluginVersion v111{1, 1, 1};
  EXPECT_LT(v100, v110);
  EXPECT_LT(v110, v111);
  EXPECT_EQ(v100, v100);
}

// ─────────────────────────────────────────────────────────────────────────────
// ManifestError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(ManifestErrorTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(ManifestError::ParseError), "Manifest parse error");
  EXPECT_EQ(to_string(ManifestError::MissingField), "Required manifest field is missing");
  EXPECT_EQ(to_string(ManifestError::InvalidVersion), "Invalid version string in manifest");
  EXPECT_EQ(to_string(ManifestError::InvalidLicense), "Invalid or unrecognized license identifier");
  EXPECT_EQ(to_string(ManifestError::InvalidExtension), "Malformed extension entry in manifest");
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_plugin_manifest
// ─────────────────────────────────────────────────────────────────────────────

constexpr std::string_view kValidManifest = R"(
name = "horcrux-wasm"
version = "1.2.3"
author = "Alice"
license = "MIT"
description = "WebAssembly support"
min_horcrux_version = "0.1.0"
max_horcrux_version = "2.0.0"
permissions.filesystem_read = true
permissions.network_access = false

[extension]
kind = "rule"
name = "wasm_binary"

[extension]
kind = "toolchain"
name = "wasm-tools"
)";

TEST(ParsePluginManifestTest, ParsesValidManifest) {
  auto result = parse_plugin_manifest(kValidManifest);
  ASSERT_TRUE(result.has_value()) << to_string(result.error());
  EXPECT_EQ(result->name, "horcrux-wasm");
  EXPECT_EQ(result->version.to_string(), "1.2.3");
  EXPECT_EQ(result->author, "Alice");
  EXPECT_EQ(result->license, "MIT");
  EXPECT_EQ(result->description, "WebAssembly support");
  EXPECT_EQ(result->min_horcrux_version.to_string(), "0.1.0");
  EXPECT_EQ(result->max_horcrux_version.to_string(), "2.0.0");
  EXPECT_TRUE(result->permissions.filesystem_read);
  EXPECT_FALSE(result->permissions.network_access);
  ASSERT_EQ(result->extensions.size(), 2u);
  EXPECT_EQ(result->extensions[0].kind, "rule");
  EXPECT_EQ(result->extensions[0].name, "wasm_binary");
  EXPECT_EQ(result->extensions[1].kind, "toolchain");
  EXPECT_EQ(result->extensions[1].name, "wasm-tools");
}

TEST(ParsePluginManifestTest, MissingNameReturnsError) {
  const std::string_view manifest = R"(
version = "1.0.0"
license = "MIT"
)";
  auto result = parse_plugin_manifest(manifest);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ManifestError::MissingField);
}

TEST(ParsePluginManifestTest, MissingLicenseReturnsError) {
  const std::string_view manifest = R"(
name = "my-plugin"
version = "1.0.0"
)";
  auto result = parse_plugin_manifest(manifest);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ManifestError::MissingField);
}

TEST(ParsePluginManifestTest, InvalidVersionReturnsError) {
  const std::string_view manifest = R"(
name = "bad-version-plugin"
version = "not-a-version"
license = "MIT"
)";
  auto result = parse_plugin_manifest(manifest);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ManifestError::InvalidVersion);
}

TEST(ParsePluginManifestTest, CommentsAndBlankLinesAreSkipped) {
  const std::string_view manifest = R"(
# This is a comment
name = "test-plugin"

# Another comment
version = "0.1.0"
license = "Apache-2.0"
)";
  auto result = parse_plugin_manifest(manifest);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->name, "test-plugin");
}

TEST(ParsePluginManifestTest, OptionalChecksumParsed) {
  const std::string_view manifest = R"(
name = "checksum-plugin"
version = "1.0.0"
license = "MIT"
checksum = "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"
)";
  auto result = parse_plugin_manifest(manifest);
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->checksum.has_value());
  EXPECT_EQ(*result->checksum, "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");
}

// ─────────────────────────────────────────────────────────────────────────────
// validate_plugin_manifest
// ─────────────────────────────────────────────────────────────────────────────

TEST(ValidatePluginManifestTest, CompatibleVersionPassesValidation) {
  PluginManifest manifest;
  manifest.name = "test";
  manifest.license = "MIT";
  manifest.min_horcrux_version = PluginVersion{0, 1, 0};
  manifest.max_horcrux_version = PluginVersion{2, 0, 0};

  PluginVersion current{0, 1, 0};
  auto error = validate_plugin_manifest(manifest, current);
  EXPECT_FALSE(error.has_value());
}

TEST(ValidatePluginManifestTest, TooOldHorcruxVersionFails) {
  PluginManifest manifest;
  manifest.name = "test";
  manifest.license = "MIT";
  manifest.min_horcrux_version = PluginVersion{1, 0, 0};

  PluginVersion current{0, 9, 0};
  auto error = validate_plugin_manifest(manifest, current);
  ASSERT_TRUE(error.has_value());
  EXPECT_NE(error->find("requires Horcrux >="), std::string::npos);
}

TEST(ValidatePluginManifestTest, TooNewHorcruxVersionFails) {
  PluginManifest manifest;
  manifest.name = "test";
  manifest.license = "MIT";
  manifest.min_horcrux_version = PluginVersion{0, 1, 0};
  manifest.max_horcrux_version = PluginVersion{0, 5, 0};

  PluginVersion current{1, 0, 0};
  auto error = validate_plugin_manifest(manifest, current);
  ASSERT_TRUE(error.has_value());
  EXPECT_NE(error->find("only compatible up to"), std::string::npos);
}

TEST(ValidatePluginManifestTest, ZeroMaxVersionIsIgnored) {
  PluginManifest manifest;
  manifest.name = "test";
  manifest.license = "MIT";
  manifest.min_horcrux_version = PluginVersion{0, 1, 0};
  manifest.max_horcrux_version = PluginVersion{0, 0, 0}; // not set

  PluginVersion current{99, 0, 0};
  auto error = validate_plugin_manifest(manifest, current);
  EXPECT_FALSE(error.has_value());
}

TEST(ValidatePluginManifestTest, InvalidExtensionKindFails) {
  PluginManifest manifest;
  manifest.name = "bad-ext";
  manifest.license = "MIT";
  manifest.extensions.push_back({.kind = "unknown_kind", .name = "something"});

  auto error = validate_plugin_manifest(manifest, PluginVersion{0, 1, 0});
  ASSERT_TRUE(error.has_value());
  EXPECT_NE(error->find("unknown extension kind"), std::string::npos);
}

} // namespace horcrux::core::test
