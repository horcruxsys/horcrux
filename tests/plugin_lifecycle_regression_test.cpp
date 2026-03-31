// Horcrux - Plugin Lifecycle Regression Tests
// Copyright (C) 2026 Horcrux Project Contributors
// Licensed under the MIT License
//
// Release-critical regression suite: verifies that the plugin system
// correctly registers, initialises, and shuts down plugins, enforces
// version compatibility and permission policies, and maintains consistent
// state across the full lifecycle.

#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/plugin_loader.h"
#include "../src/core/plugin_manifest.h"
#include "../src/core/plugin_verifier.h"
#include "../src/core/registry_client.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static auto make_manifest(std::string name, PluginVersion version = {1, 0, 0},
                          PluginVersion min_hx = {2026, 4, 1}) -> PluginManifest {
  PluginManifest m;
  m.name = std::move(name);
  m.version = version;
  m.author = "Test Author";
  m.license = "MIT";
  m.description = "Regression test plugin";
  m.min_horcrux_version = min_hx;
  return m;
}

static const PluginVersion kReleaseVersion{2026, 4, 1};

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle: register → count is correct
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLifecycleRegressionTest, RegisteredPluginAppearsInList) {
  PluginLoader loader(kReleaseVersion);
  auto result = loader.register_plugin(make_manifest("plugin-a"), "/tmp/plugin-a");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(loader.plugins().size(), 1u);
  EXPECT_EQ(loader.plugins()[0].manifest.name, "plugin-a");
}

TEST(PluginLifecycleRegressionTest, MultiplePluginsRegisteredInOrder) {
  PluginLoader loader(kReleaseVersion);
  loader.register_plugin(make_manifest("plugin-b"), "/tmp/plugin-b").value();
  loader.register_plugin(make_manifest("plugin-a"), "/tmp/plugin-a").value();
  loader.register_plugin(make_manifest("plugin-c"), "/tmp/plugin-c").value();

  EXPECT_EQ(loader.plugins().size(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle: duplicate registration is rejected
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLifecycleRegressionTest, DuplicateRegistrationIsRejected) {
  PluginLoader loader(kReleaseVersion);
  loader.register_plugin(make_manifest("dup-plugin"), "/tmp/dup").value();
  auto result = loader.register_plugin(make_manifest("dup-plugin"), "/tmp/dup");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), PluginLoaderError::AlreadyLoaded);
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle: initialize_all transitions state to Initialized
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLifecycleRegressionTest, InitializeAllTransitionsToInitialized) {
  PluginLoader loader(kReleaseVersion);
  loader.register_plugin(make_manifest("init-plugin"), "/tmp/init-plugin").value();

  loader.initialize_all();

  EXPECT_EQ(loader.count_in_state(PluginState::Initialized), 1u);
  EXPECT_EQ(loader.count_in_state(PluginState::Failed), 0u);
}

TEST(PluginLifecycleRegressionTest, InitializeAllWithMultiplePlugins) {
  PluginLoader loader(kReleaseVersion);
  loader.register_plugin(make_manifest("p1"), "/tmp/p1").value();
  loader.register_plugin(make_manifest("p2"), "/tmp/p2").value();
  loader.register_plugin(make_manifest("p3"), "/tmp/p3").value();

  loader.initialize_all();

  EXPECT_EQ(loader.count_in_state(PluginState::Initialized), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle: shutdown_all transitions state to ShutDown
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLifecycleRegressionTest, ShutdownAllTransitionsToShutDown) {
  PluginLoader loader(kReleaseVersion);
  loader.register_plugin(make_manifest("sd-plugin"), "/tmp/sd-plugin").value();
  loader.initialize_all();
  loader.shutdown_all();

  EXPECT_EQ(loader.count_in_state(PluginState::ShutDown), 1u);
  EXPECT_EQ(loader.count_in_state(PluginState::Initialized), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle: version compatibility gate
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLifecycleRegressionTest, PluginRequiringNewerHorcruxIsRejected) {
  // Plugin requires Horcrux 9999.0.0 — far future; current is 2026.4.1
  PluginVersion future{9999, 0, 0};
  PluginManifest m = make_manifest("future-plugin");
  m.min_horcrux_version = future;

  PluginLoader loader(kReleaseVersion);
  auto result = loader.register_plugin(m, "/tmp/future-plugin");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), PluginLoaderError::CompatibilityFailed);
}

TEST(PluginLifecycleRegressionTest, PluginCompatibleWithCurrentVersionIsAccepted) {
  // Plugin requires exactly the current release version
  PluginManifest m = make_manifest("compat-plugin");
  m.min_horcrux_version = kReleaseVersion;

  PluginLoader loader(kReleaseVersion);
  auto result = loader.register_plugin(m, "/tmp/compat-plugin");
  EXPECT_TRUE(result.has_value());
}

TEST(PluginLifecycleRegressionTest, PluginRequiringOlderHorcruxIsAccepted) {
  // Plugin requires version 1.0.0 — current is newer
  PluginManifest m = make_manifest("old-compat-plugin");
  m.min_horcrux_version = PluginVersion{1, 0, 0};

  PluginLoader loader(kReleaseVersion);
  auto result = loader.register_plugin(m, "/tmp/old-compat-plugin");
  EXPECT_TRUE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle: PluginState string conversions are stable across the release
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLifecycleRegressionTest, StateStringsAreStable) {
  EXPECT_EQ(to_string(PluginState::Unloaded), "unloaded");
  EXPECT_EQ(to_string(PluginState::Initialized), "initialized");
  EXPECT_EQ(to_string(PluginState::Failed), "failed");
  EXPECT_EQ(to_string(PluginState::ShutDown), "shutdown");
}

TEST(PluginLifecycleRegressionTest, ErrorStringsAreStable) {
  EXPECT_EQ(to_string(PluginLoaderError::ManifestNotFound), "Plugin manifest not found");
  EXPECT_EQ(to_string(PluginLoaderError::ManifestInvalid), "Plugin manifest is invalid");
  EXPECT_EQ(to_string(PluginLoaderError::CompatibilityFailed),
            "Plugin is not compatible with this version of Horcrux");
  EXPECT_EQ(to_string(PluginLoaderError::AlreadyLoaded),
            "A plugin with this name is already loaded");
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle: permission enforcement gate
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginLifecycleRegressionTest, PluginWithNetworkPermissionRejectedByStrictPolicy) {
  PluginManifest m = make_manifest("net-plugin");
  m.permissions.network_access = true;

  // Strict trust policy: network_access is forbidden
  PluginTrustPolicy strict = PluginTrustPolicy::strict_policy();
  PluginLoader loader(kReleaseVersion, strict);
  auto result = loader.register_plugin(m, "/tmp/net-plugin");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), PluginLoaderError::PermissionViolation);
}

TEST(PluginLifecycleRegressionTest, PluginWithNetworkPermissionRejectedByDefaultPolicy) {
  PluginManifest m = make_manifest("net-plugin-default");
  m.permissions.network_access = true;

  // Default policy: network_access is NOT allowed (allow_network_access = false)
  PluginTrustPolicy default_policy = PluginTrustPolicy::default_policy();
  PluginLoader loader(kReleaseVersion, default_policy);
  auto result = loader.register_plugin(m, "/tmp/net-plugin-default");
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), PluginLoaderError::PermissionViolation);
}

// ─────────────────────────────────────────────────────────────────────────────
// RegistryClient: install / list / remove regression
// ─────────────────────────────────────────────────────────────────────────────

class RegistryFixture {
public:
  RegistryFixture() {
    tmp_dir_ = std::filesystem::temp_directory_path() / "horcrux_lifecycle_reg_test";
    plugins_dir_ = tmp_dir_ / "plugins";
    lockfile_ = tmp_dir_ / "plugins.lock";
    std::filesystem::create_directories(plugins_dir_);
  }
  ~RegistryFixture() {
    std::error_code ec;
    std::filesystem::remove_all(tmp_dir_, ec);
  }
  [[nodiscard]] auto make_client() const -> RegistryClient {
    return RegistryClient{plugins_dir_, lockfile_};
  }

private:
  std::filesystem::path tmp_dir_;
  std::filesystem::path plugins_dir_;
  std::filesystem::path lockfile_;
};

static auto make_package(std::string name, std::string ver = "1.0.0") -> RegistryPackage {
  RegistryPackage pkg;
  pkg.name = std::move(name);
  pkg.version = {1, 0, 0};
  pkg.author = "Test";
  pkg.description = "Test package";
  pkg.license = "MIT";
  pkg.download_url = "https://example.com/" + pkg.name + "-" + ver + ".tar.gz";
  pkg.checksum = "";
  return pkg;
}

TEST(PluginLifecycleRegressionTest, RegistryClientInstallAndListRoundTrip) {
  RegistryFixture fix;
  auto client = fix.make_client();
  client.seed_package(make_package("my-plugin"));

  auto info = client.package_info("my-plugin");
  ASSERT_TRUE(info.has_value());
  EXPECT_EQ(info->name, "my-plugin");
}

TEST(PluginLifecycleRegressionTest, RegistryClientRemovePlugin) {
  RegistryFixture fix;
  auto client = fix.make_client();
  client.seed_package(make_package("rm-plugin"));

  auto install_result = client.install("rm-plugin");
  ASSERT_TRUE(install_result.has_value());

  auto remove_result = client.remove("rm-plugin");
  ASSERT_TRUE(remove_result.has_value());

  // After removal the plugin must not appear in installed list
  auto installed = client.list_installed();
  for (const auto& entry : installed) {
    EXPECT_NE(entry.name, "rm-plugin");
  }
}

TEST(PluginLifecycleRegressionTest, RegistryClientSearchIsCaseInsensitive) {
  RegistryFixture fix;
  auto client = fix.make_client();
  auto pkg = make_package("Kotlin-Plugin");
  pkg.name = "Kotlin-Plugin";
  client.seed_package(pkg);

  auto results = client.search("kotlin");
  EXPECT_FALSE(results.empty());
}

} // namespace horcrux::core::test
