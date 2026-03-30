// Horcrux - Plugin & Registry CLI Command Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/cli/logger.h"
#include "../src/cli/plugin_command.h"
#include "../src/cli/registry_command.h"

namespace horcrux::cli::test {

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build fake argv
// ─────────────────────────────────────────────────────────────────────────────

class FakeArgv {
public:
  explicit FakeArgv(std::vector<std::string> args) : args_(std::move(args)) {
    for (auto& a : args_) {
      ptrs_.push_back(const_cast<char*>(a.c_str())); // NOLINT
    }
  }
  auto argc() const -> int { return static_cast<int>(ptrs_.size()); }
  auto argv() -> char** { return ptrs_.data(); }

private:
  std::vector<std::string> args_;
  std::vector<char*> ptrs_;
};

// ─────────────────────────────────────────────────────────────────────────────
// PluginCommandError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginCommandErrorTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(PluginCommandError::NoSubcommand),
            "No plugin subcommand provided");
  EXPECT_EQ(to_string(PluginCommandError::InvalidSubcommand),
            "Invalid plugin subcommand");
  EXPECT_EQ(to_string(PluginCommandError::MissingArgument),
            "Missing required argument");
  EXPECT_EQ(to_string(PluginCommandError::OperationFailed), "Plugin operation failed");
}

// ─────────────────────────────────────────────────────────────────────────────
// RegistryCommandError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(RegistryCommandErrorTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(RegistryCommandError::NoSubcommand),
            "No registry subcommand provided");
  EXPECT_EQ(to_string(RegistryCommandError::InvalidSubcommand),
            "Invalid registry subcommand");
  EXPECT_EQ(to_string(RegistryCommandError::MissingArgument),
            "Missing required argument");
  EXPECT_EQ(to_string(RegistryCommandError::OperationFailed),
            "Registry operation failed");
}

// ─────────────────────────────────────────────────────────────────────────────
// plugin --help / no subcommand
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginCommandTest, NoSubcommandReturnsNonZero) {
  Logger logger;
  FakeArgv args({"horcrux", "plugin"});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(PluginCommandTest, HelpFlagReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "plugin", "--help"});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(PluginCommandTest, HelpSubcommandReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "plugin", "help"});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(PluginCommandTest, UnknownSubcommandReturnsNonZero) {
  Logger logger;
  FakeArgv args({"horcrux", "plugin", "frobnicate"});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// plugin search (empty registry → 0 results, still succeeds)
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginCommandTest, SearchEmptyRegistryReturnsZero) {
  Logger logger;
  const fs::path tmp = fs::temp_directory_path() / "horcrux_plugin_cmd_test_search";
  const fs::path plugins_dir = tmp / "plugins";
  const fs::path lockfile = tmp / "plugins.lock";
  fs::create_directories(plugins_dir);

  FakeArgv args({"horcrux", "plugin", "search", "wasm",
                 "--plugins-dir=" + plugins_dir.string(),
                 "--lockfile=" + lockfile.string()});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);

  fs::remove_all(tmp);
}

// ─────────────────────────────────────────────────────────────────────────────
// plugin list (empty → 0)
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginCommandTest, ListEmptyReturnsZero) {
  Logger logger;
  const fs::path tmp = fs::temp_directory_path() / "horcrux_plugin_cmd_test_list";
  const fs::path plugins_dir = tmp / "plugins";
  const fs::path lockfile = tmp / "plugins.lock";
  fs::create_directories(plugins_dir);

  FakeArgv args({"horcrux", "plugin", "list", "--plugins-dir=" + plugins_dir.string(),
                 "--lockfile=" + lockfile.string()});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);

  fs::remove_all(tmp);
}

// ─────────────────────────────────────────────────────────────────────────────
// plugin install – missing name returns error
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginCommandTest, InstallMissingNameReturnsNonZero) {
  Logger logger;
  FakeArgv args({"horcrux", "plugin", "install"});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// plugin install – unknown package returns error
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginCommandTest, InstallUnknownPackageReturnsNonZero) {
  Logger logger;
  const fs::path tmp = fs::temp_directory_path() / "horcrux_plugin_cmd_test_install";
  const fs::path plugins_dir = tmp / "plugins";
  const fs::path lockfile = tmp / "plugins.lock";
  fs::create_directories(plugins_dir);

  FakeArgv args({"horcrux", "plugin", "install", "nonexistent-package",
                 "--plugins-dir=" + plugins_dir.string(),
                 "--lockfile=" + lockfile.string()});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);

  fs::remove_all(tmp);
}

// ─────────────────────────────────────────────────────────────────────────────
// plugin info – missing name returns error
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginCommandTest, InfoMissingNameReturnsNonZero) {
  Logger logger;
  FakeArgv args({"horcrux", "plugin", "info"});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// plugin remove – missing name returns error
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginCommandTest, RemoveMissingNameReturnsNonZero) {
  Logger logger;
  FakeArgv args({"horcrux", "plugin", "remove"});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// plugin update – missing name returns error
// ─────────────────────────────────────────────────────────────────────────────

TEST(PluginCommandTest, UpdateMissingNameReturnsNonZero) {
  Logger logger;
  FakeArgv args({"horcrux", "plugin", "update"});
  int rc = handle_plugin_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// registry --help / no subcommand
// ─────────────────────────────────────────────────────────────────────────────

TEST(RegistryCommandTest, NoSubcommandReturnsNonZero) {
  Logger logger;
  FakeArgv args({"horcrux", "registry"});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(RegistryCommandTest, HelpFlagReturnsZero) {
  Logger logger;
  FakeArgv args({"horcrux", "registry", "--help"});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);
}

TEST(RegistryCommandTest, ListEmptyReturnsZero) {
  Logger logger;
  const fs::path tmp = fs::temp_directory_path() / "horcrux_registry_cmd_test_list";
  fs::create_directories(tmp);
  const fs::path config = tmp / "registries.conf";

  FakeArgv args({"horcrux", "registry", "list", "--config=" + config.string()});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_EQ(rc, 0);

  fs::remove_all(tmp);
}

TEST(RegistryCommandTest, AddAndListRegistry) {
  Logger logger;
  const fs::path tmp = fs::temp_directory_path() / "horcrux_registry_cmd_test_add";
  fs::create_directories(tmp);
  const fs::path config = tmp / "registries.conf";

  {
    FakeArgv args({"horcrux", "registry", "add", "myregistry",
                   "https://registry.example.com", "--config=" + config.string()});
    int rc = handle_registry_command(args.argc(), args.argv(), logger);
    EXPECT_EQ(rc, 0);
  }

  {
    FakeArgv args({"horcrux", "registry", "list", "--config=" + config.string()});
    int rc = handle_registry_command(args.argc(), args.argv(), logger);
    EXPECT_EQ(rc, 0);
  }

  fs::remove_all(tmp);
}

TEST(RegistryCommandTest, RemoveNonexistentRegistryReturnsNonZero) {
  Logger logger;
  const fs::path tmp = fs::temp_directory_path() / "horcrux_registry_cmd_test_remove";
  fs::create_directories(tmp);
  const fs::path config = tmp / "registries.conf";

  FakeArgv args(
      {"horcrux", "registry", "remove", "nonexistent", "--config=" + config.string()});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);

  fs::remove_all(tmp);
}

TEST(RegistryCommandTest, AddMissingArgsReturnsNonZero) {
  Logger logger;
  FakeArgv args({"horcrux", "registry", "add", "onlyname"});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

TEST(RegistryCommandTest, UnknownSubcommandReturnsNonZero) {
  Logger logger;
  FakeArgv args({"horcrux", "registry", "frobnicate"});
  int rc = handle_registry_command(args.argc(), args.argv(), logger);
  EXPECT_NE(rc, 0);
}

} // namespace horcrux::cli::test
