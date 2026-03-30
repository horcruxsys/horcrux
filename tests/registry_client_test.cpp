// Horcrux - Registry Client Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/core/registry_client.h"

namespace horcrux::core::test {

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static auto make_package(std::string name, PluginVersion version = {1, 0, 0})
    -> RegistryPackage {
  RegistryPackage pkg;
  pkg.name = std::move(name);
  pkg.version = version;
  pkg.author = "Test Author";
  pkg.description = "A test plugin";
  pkg.license = "MIT";
  pkg.download_url = "https://registry.example.com/" + pkg.name;
  pkg.checksum = "";
  return pkg;
}

class RegistryClientTest : public ::testing::Test {
protected:
  void SetUp() override {
    tmp_dir_ = fs::temp_directory_path() / "horcrux_registry_test";
    plugins_dir_ = tmp_dir_ / "plugins";
    lockfile_path_ = tmp_dir_ / "plugins.lock";
    fs::create_directories(plugins_dir_);
  }

  void TearDown() override { fs::remove_all(tmp_dir_); }

  fs::path tmp_dir_;
  fs::path plugins_dir_;
  fs::path lockfile_path_;
};

// ─────────────────────────────────────────────────────────────────────────────
// RegistryError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(RegistryErrorTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(RegistryError::NetworkUnavailable), "Registry network unavailable");
  EXPECT_EQ(to_string(RegistryError::PackageNotFound), "Package not found in registry");
  EXPECT_EQ(to_string(RegistryError::VersionNotFound),
            "Requested version not found in registry");
  EXPECT_EQ(to_string(RegistryError::DownloadFailed), "Package download failed");
  EXPECT_EQ(to_string(RegistryError::ChecksumMismatch),
            "Downloaded package checksum mismatch");
  EXPECT_EQ(to_string(RegistryError::InstallFailed), "Package installation failed");
  EXPECT_EQ(to_string(RegistryError::AlreadyInstalled),
            "Package is already installed at this version");
  EXPECT_EQ(to_string(RegistryError::LockfileError),
            "Failed to read or write plugin lockfile");
  EXPECT_EQ(to_string(RegistryError::InvalidConfig), "Registry configuration is invalid");
}

// ─────────────────────────────────────────────────────────────────────────────
// Registry management
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RegistryClientTest, AddAndListRegistries) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.add_registry({.name = "official", .url = "https://registry.horcrux.dev"});
  client.add_registry({.name = "mirror", .url = "https://mirror.example.com"});

  const auto& regs = client.list_registries();
  ASSERT_EQ(regs.size(), 2u);
  EXPECT_EQ(regs[0].name, "official");
  EXPECT_EQ(regs[1].name, "mirror");
}

TEST_F(RegistryClientTest, RemoveRegistry) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.add_registry({.name = "r1", .url = "https://r1.example.com"});
  client.add_registry({.name = "r2", .url = "https://r2.example.com"});

  EXPECT_TRUE(client.remove_registry("r1"));
  EXPECT_EQ(client.list_registries().size(), 1u);
  EXPECT_EQ(client.list_registries()[0].name, "r2");
}

TEST_F(RegistryClientTest, RemoveNonexistentRegistryReturnsFalse) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  EXPECT_FALSE(client.remove_registry("does-not-exist"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Package index / search
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RegistryClientTest, SearchByNameSubstring) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("horcrux-wasm"));
  client.seed_package(make_package("horcrux-rust"));
  client.seed_package(make_package("other-plugin"));

  auto results = client.search("wasm");
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].name, "horcrux-wasm");
}

TEST_F(RegistryClientTest, SearchEmptyQueryReturnsAll) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("a"));
  client.seed_package(make_package("b"));

  auto results = client.search("");
  EXPECT_EQ(results.size(), 2u);
}

TEST_F(RegistryClientTest, SearchCaseInsensitive) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("HoRcRuX-WaSm"));

  auto results = client.search("wasm");
  EXPECT_EQ(results.size(), 1u);
}

TEST_F(RegistryClientTest, PackageInfoReturnsMetadata) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("test-pkg"));

  auto info = client.package_info("test-pkg");
  ASSERT_TRUE(info.has_value());
  EXPECT_EQ(info->name, "test-pkg");
}

TEST_F(RegistryClientTest, PackageInfoMissingReturnsNullopt) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  EXPECT_FALSE(client.package_info("nonexistent").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Install
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RegistryClientTest, InstallSucceeds) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("test-pkg"));

  auto result = client.install("test-pkg");
  ASSERT_TRUE(result.has_value()) << to_string(result.error());
  EXPECT_EQ(result->name, "test-pkg");

  // Manifest should be on disk
  EXPECT_TRUE(fs::exists(plugins_dir_ / "test-pkg" / "manifest.toml"));
}

TEST_F(RegistryClientTest, InstallUnknownPackageFails) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  auto result = client.install("does-not-exist");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), RegistryError::PackageNotFound);
}

TEST_F(RegistryClientTest, InstallAlreadyInstalledFails) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("dup-pkg"));

  auto r1 = client.install("dup-pkg");
  ASSERT_TRUE(r1.has_value());

  auto r2 = client.install("dup-pkg");
  ASSERT_FALSE(r2.has_value());
  EXPECT_EQ(r2.error(), RegistryError::AlreadyInstalled);
}

TEST_F(RegistryClientTest, InstallSpecificVersionSucceeds) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("versioned", {1, 0, 0}));
  client.seed_package(make_package("versioned", {2, 0, 0}));

  PluginVersion v1{1, 0, 0};
  auto result = client.install("versioned", v1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->version.to_string(), "1.0.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// Remove
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RegistryClientTest, RemoveInstalledPlugin) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("to-remove"));
  auto install_result = client.install("to-remove");
  ASSERT_TRUE(install_result.has_value());

  auto remove_result = client.remove("to-remove");
  ASSERT_TRUE(remove_result.has_value()) << to_string(remove_result.error());
  EXPECT_FALSE(fs::exists(plugins_dir_ / "to-remove"));
}

TEST_F(RegistryClientTest, RemoveNoninstalledPluginFails) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  auto result = client.remove("not-installed");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), RegistryError::PackageNotFound);
}

// ─────────────────────────────────────────────────────────────────────────────
// Update
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RegistryClientTest, UpdatePlugin) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("update-me", {1, 0, 0}));
  client.seed_package(make_package("update-me", {2, 0, 0}));

  auto install_result = client.install("update-me", PluginVersion{1, 0, 0});
  ASSERT_TRUE(install_result.has_value());

  auto update_result = client.update("update-me");
  ASSERT_TRUE(update_result.has_value()) << to_string(update_result.error());
  EXPECT_EQ(update_result->version.to_string(), "2.0.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// Lockfile
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RegistryClientTest, LockfileRoundtrip) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("lock-pkg"));
  auto install_result = client.install("lock-pkg");
  ASSERT_TRUE(install_result.has_value());

  // Read back the lockfile
  auto lockfile = client.read_lockfile();
  ASSERT_EQ(lockfile.entries.size(), 1u);
  EXPECT_EQ(lockfile.entries[0].name, "lock-pkg");
}

TEST_F(RegistryClientTest, ReadLockfileMissingReturnsEmpty) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  auto lockfile = client.read_lockfile();
  EXPECT_TRUE(lockfile.entries.empty());
}

TEST_F(RegistryClientTest, ListInstalledReflectsLockfile) {
  RegistryClient client(plugins_dir_, lockfile_path_);
  client.seed_package(make_package("p1"));
  client.seed_package(make_package("p2"));
  auto r1 = client.install("p1");
  auto r2 = client.install("p2");
  ASSERT_TRUE(r1.has_value());
  ASSERT_TRUE(r2.has_value());

  auto installed = client.list_installed();
  EXPECT_EQ(installed.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// PluginLockfile::find
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLockfileTest, FindByName) {
  PluginLockfile lockfile;
  lockfile.entries.push_back({.name = "foo", .version = {1, 0, 0}, .checksum = "abc"});
  lockfile.entries.push_back({.name = "bar", .version = {2, 0, 0}, .checksum = "def"});

  const auto* foo = lockfile.find("foo");
  ASSERT_NE(foo, nullptr);
  EXPECT_EQ(foo->name, "foo");

  EXPECT_EQ(lockfile.find("missing"), nullptr);
}

} // namespace horcrux::core::test
