// Horcrux - Plugin Loader Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/core/plugin_loader.h"

namespace horcrux::core::test {

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Build a minimal valid manifest
static auto make_manifest(std::string name, PluginVersion version = {1, 0, 0})
    -> PluginManifest {
  PluginManifest m;
  m.name = std::move(name);
  m.version = version;
  m.author = "Test Author";
  m.license = "MIT";
  m.description = "Test plugin";
  m.min_horcrux_version = PluginVersion{0, 1, 0};
  return m;
}

/// Current Horcrux version used in all tests
static const PluginVersion kHorcruxVersion{0, 1, 0};

// ─────────────────────────────────────────────────────────────────────────────
// PluginState to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginStateTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(PluginState::Unloaded), "unloaded");
  EXPECT_EQ(to_string(PluginState::Initialized), "initialized");
  EXPECT_EQ(to_string(PluginState::Failed), "failed");
  EXPECT_EQ(to_string(PluginState::ShutDown), "shutdown");
}

// ─────────────────────────────────────────────────────────────────────────────
// PluginLoaderError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLoaderErrorTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(PluginLoaderError::ManifestNotFound), "Plugin manifest not found");
  EXPECT_EQ(to_string(PluginLoaderError::ManifestInvalid), "Plugin manifest is invalid");
  EXPECT_EQ(to_string(PluginLoaderError::CompatibilityFailed),
            "Plugin is not compatible with this version of Horcrux");
  EXPECT_EQ(to_string(PluginLoaderError::ChecksumFailed),
            "Plugin checksum verification failed");
  EXPECT_EQ(to_string(PluginLoaderError::PermissionViolation),
            "Plugin requests permissions not allowed by trust policy");
  EXPECT_EQ(to_string(PluginLoaderError::AlreadyLoaded),
            "A plugin with this name is already loaded");
  EXPECT_EQ(to_string(PluginLoaderError::InitFailed), "Plugin initialization failed");
}

// ─────────────────────────────────────────────────────────────────────────────
// register_plugin
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLoaderTest, RegisterValidPlugin) {
  PluginLoader loader(kHorcruxVersion);
  auto result = loader.register_plugin(make_manifest("test-plugin"), "/tmp/test-plugin");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(loader.plugins().size(), 1u);
  EXPECT_EQ(loader.plugins()[0].manifest.name, "test-plugin");
  EXPECT_EQ(loader.plugins()[0].state, PluginState::Unloaded);
}

TEST(PluginLoaderTest, RegisterDuplicatePluginFails) {
  PluginLoader loader(kHorcruxVersion);
  auto r1 = loader.register_plugin(make_manifest("dup"), "/tmp/dup");
  ASSERT_TRUE(r1.has_value());
  auto r2 = loader.register_plugin(make_manifest("dup"), "/tmp/dup");
  ASSERT_FALSE(r2.has_value());
  EXPECT_EQ(r2.error(), PluginLoaderError::AlreadyLoaded);
}

TEST(PluginLoaderTest, IncompatiblePluginFails) {
  PluginLoader loader(kHorcruxVersion); // horcrux 0.1.0
  auto manifest = make_manifest("future-plugin");
  manifest.min_horcrux_version = PluginVersion{9, 0, 0}; // requires 9.0.0+

  auto result = loader.register_plugin(std::move(manifest), "/tmp/future");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), PluginLoaderError::CompatibilityFailed);
}

TEST(PluginLoaderTest, PermissionViolationFails) {
  PluginTrustPolicy strict = PluginTrustPolicy::strict_policy();
  PluginLoader loader(kHorcruxVersion, strict);

  auto manifest = make_manifest("network-plugin");
  manifest.permissions.network_access = true;

  auto result = loader.register_plugin(std::move(manifest), "/tmp/net");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), PluginLoaderError::PermissionViolation);
}

// ─────────────────────────────────────────────────────────────────────────────
// initialize_all / shutdown_all
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLoaderTest, InitializeAllSetsInitializedState) {
  PluginLoader loader(kHorcruxVersion);
  auto r1 = loader.register_plugin(make_manifest("alpha"), "/tmp/alpha");
  auto r2 = loader.register_plugin(make_manifest("beta"), "/tmp/beta");
  ASSERT_TRUE(r1.has_value());
  ASSERT_TRUE(r2.has_value());
  loader.initialize_all();

  EXPECT_EQ(loader.count_in_state(PluginState::Initialized), 2u);
  EXPECT_EQ(loader.count_in_state(PluginState::Unloaded), 0u);
}

TEST(PluginLoaderTest, ShutdownAllSetsShutdownState) {
  PluginLoader loader(kHorcruxVersion);
  auto r1 = loader.register_plugin(make_manifest("plugin-a"), "/tmp/plugin-a");
  ASSERT_TRUE(r1.has_value());
  loader.initialize_all();
  loader.shutdown_all();

  EXPECT_EQ(loader.count_in_state(PluginState::ShutDown), 1u);
  EXPECT_EQ(loader.count_in_state(PluginState::Initialized), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Deterministic load order
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLoaderTest, InitializeAllUsesAlphabeticalOrder) {
  PluginLoader loader(kHorcruxVersion);
  // Register in reverse order
  auto r1 = loader.register_plugin(make_manifest("zebra"), "/tmp/zebra");
  auto r2 = loader.register_plugin(make_manifest("apple"), "/tmp/apple");
  auto r3 = loader.register_plugin(make_manifest("mango"), "/tmp/mango");
  ASSERT_TRUE(r1.has_value());
  ASSERT_TRUE(r2.has_value());
  ASSERT_TRUE(r3.has_value());
  loader.initialize_all();

  const auto& plugins = loader.plugins();
  ASSERT_EQ(plugins.size(), 3u);
  EXPECT_EQ(plugins[0].manifest.name, "apple");
  EXPECT_EQ(plugins[1].manifest.name, "mango");
  EXPECT_EQ(plugins[2].manifest.name, "zebra");
}

// ─────────────────────────────────────────────────────────────────────────────
// find_plugin
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLoaderTest, FindPluginByName) {
  PluginLoader loader(kHorcruxVersion);
  auto r1 = loader.register_plugin(make_manifest("find-me"), "/tmp/find-me");
  ASSERT_TRUE(r1.has_value());

  const auto* p = loader.find_plugin("find-me");
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->manifest.name, "find-me");
}

TEST(PluginLoaderTest, FindPluginMissingReturnsNull) {
  PluginLoader loader(kHorcruxVersion);
  EXPECT_EQ(loader.find_plugin("nonexistent"), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// scan_directory
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLoaderTest, ScanDirectoryLoadsPlugins) {
  // Create a temporary plugins directory
  const fs::path tmp_dir = fs::temp_directory_path() / "horcrux_loader_test";
  fs::create_directories(tmp_dir / "plugin-a");
  fs::create_directories(tmp_dir / "plugin-b");

  // Write minimal manifests
  auto write_manifest = [](const fs::path& dir, const std::string& name) {
    std::ofstream f(dir / "manifest.toml");
    f << "name = \"" << name << "\"\n";
    f << "version = \"1.0.0\"\n";
    f << "author = \"Test\"\n";
    f << "license = \"MIT\"\n";
  };
  write_manifest(tmp_dir / "plugin-a", "plugin-a");
  write_manifest(tmp_dir / "plugin-b", "plugin-b");

  PluginLoader loader(kHorcruxVersion);
  size_t count = loader.scan_directory(tmp_dir);
  EXPECT_EQ(count, 2u);

  // Cleanup
  fs::remove_all(tmp_dir);
}

TEST(PluginLoaderTest, ScanNonexistentDirectoryReturnsZero) {
  PluginLoader loader(kHorcruxVersion);
  size_t count = loader.scan_directory("/nonexistent/path/to/plugins");
  EXPECT_EQ(count, 0u);
}

} // namespace horcrux::core::test
