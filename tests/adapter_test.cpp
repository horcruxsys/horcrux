// Horcrux - Adapter Interface and C++ Adapter Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/adapter.h"
#include "../src/core/adapter_registry.h"
#include "../src/core/build_graph.h"
#include "../src/core/build_node.h"
#include "../src/core/cpp_adapter.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// AdapterError to_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterErrorTest, ToStringCoversAllValues) {
  EXPECT_EQ(to_string(AdapterError::UnsupportedKind),
            "Target kind not supported by this adapter");
  EXPECT_EQ(to_string(AdapterError::InvalidConfig),
            "Invalid or incomplete target configuration");
  EXPECT_EQ(to_string(AdapterError::PlanningError), "Failed to plan build actions");
  EXPECT_EQ(to_string(AdapterError::ToolchainError),
            "Toolchain not found or misconfigured");
  EXPECT_EQ(to_string(AdapterError::CacheKeyError), "Failed to compute cache key");
}

// ─────────────────────────────────────────────────────────────────────────────
// CppAdapter – construction and identity
// ─────────────────────────────────────────────────────────────────────────────

static auto make_cpp_adapter() -> CppAdapter {
  CppToolchain tc;
  tc.compiler = "g++";
  tc.archiver = "ar";
  tc.compiler_id = "gcc";
  tc.compiler_version = "13.0.0";
  return CppAdapter{tc};
}

TEST(CppAdapterInfoTest, NameIsCpp) {
  auto adapter = make_cpp_adapter();
  EXPECT_EQ(adapter.info().name, "cpp");
}

TEST(CppAdapterInfoTest, VersionIsPresent) {
  auto adapter = make_cpp_adapter();
  EXPECT_FALSE(adapter.info().version.empty());
}

TEST(CppAdapterInfoTest, SupportedKinds) {
  auto adapter = make_cpp_adapter();
  const auto& kinds = adapter.info().supported_kinds;
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "cc_library"), kinds.end());
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "cc_binary"), kinds.end());
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "cc_test"),   kinds.end());
}

TEST(CppAdapterInfoTest, SupportsKindHelper) {
  auto adapter = make_cpp_adapter();
  EXPECT_TRUE(adapter.supports_kind("cc_library"));
  EXPECT_TRUE(adapter.supports_kind("cc_binary"));
  EXPECT_TRUE(adapter.supports_kind("cc_test"));
  EXPECT_FALSE(adapter.supports_kind("java_library"));
  EXPECT_FALSE(adapter.supports_kind("py_binary"));
}

// ─────────────────────────────────────────────────────────────────────────────
// CppAdapter – parse_target
// ─────────────────────────────────────────────────────────────────────────────

TEST(CppAdapterParseTest, ParsesLibraryNode) {
  auto adapter = make_cpp_adapter();
  BuildNode node("//pkg:mylib", "cc_library",
                 {"pkg/lib.cpp", "pkg/lib.h"},
                 {"pkg/libmylib.a"},
                 {{"copts", "-Wall"}, {"includes", "pkg/include"}});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->label, "//pkg:mylib");
  EXPECT_EQ(result->kind,  "cc_library");
}

TEST(CppAdapterParseTest, ParsesBinaryNode) {
  auto adapter = make_cpp_adapter();
  BuildNode node("//pkg:mybin", "cc_binary",
                 {"pkg/main.cpp"}, {"pkg/mybin"}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->kind, "cc_binary");
}

TEST(CppAdapterParseTest, ParsesTestNode) {
  auto adapter = make_cpp_adapter();
  BuildNode node("//pkg:mytest", "cc_test",
                 {"pkg/test_main.cpp"}, {"pkg/mytest"}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->kind, "cc_test");
}

TEST(CppAdapterParseTest, RejectsUnsupportedKind) {
  auto adapter = make_cpp_adapter();
  BuildNode node("//pkg:myjar", "java_library", {}, {}, {});

  auto result = adapter.parse_target(node);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AdapterError::UnsupportedKind);
}

// ─────────────────────────────────────────────────────────────────────────────
// CppAdapter – plan_actions
// ─────────────────────────────────────────────────────────────────────────────

static auto build_empty_graph() -> BuildGraph {
  auto builder = BuildGraph::builder();
  return builder.build().value();
}

TEST(CppAdapterPlanTest, LibraryPlanHasCompileAndArchiveActions) {
  auto adapter = make_cpp_adapter();
  BuildNode node("//pkg:mylib", "cc_library",
                 {"pkg/lib.cpp"}, {"pkg/libmylib.a"}, {});

  auto target = adapter.parse_target(node).value();
  auto graph  = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  // Should have at least 1 compile + 1 archive
  EXPECT_GE(actions->size(), 2u);

  bool has_compile = false;
  bool has_archive = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Compile) has_compile = true;
    if (a.kind == BuildAction::Kind::Archive) has_archive = true;
  }
  EXPECT_TRUE(has_compile);
  EXPECT_TRUE(has_archive);
}

TEST(CppAdapterPlanTest, BinaryPlanHasCompileAndLinkActions) {
  auto adapter = make_cpp_adapter();
  BuildNode node("//pkg:mybin", "cc_binary",
                 {"pkg/main.cpp"}, {"pkg/mybin"}, {});

  auto target  = adapter.parse_target(node).value();
  auto graph   = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  bool has_compile = false;
  bool has_link    = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Compile) has_compile = true;
    if (a.kind == BuildAction::Kind::Link)    has_link = true;
  }
  EXPECT_TRUE(has_compile);
  EXPECT_TRUE(has_link);
}

TEST(CppAdapterPlanTest, HeaderOnlyLibraryProducesNoActions) {
  auto adapter = make_cpp_adapter();
  // No .cpp sources — header-only library
  BuildNode node("//pkg:headers", "cc_library",
                 {"pkg/lib.h"}, {}, {});

  auto target  = adapter.parse_target(node).value();
  auto graph   = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  // No compile or archive needed for header-only
  EXPECT_EQ(actions->size(), 0u);
}

TEST(CppAdapterPlanTest, CompileActionIncludesSourceFile) {
  auto adapter = make_cpp_adapter();
  BuildNode node("//pkg:lib", "cc_library",
                 {"pkg/lib.cpp"}, {"pkg/liblib.a"}, {});

  auto target  = adapter.parse_target(node).value();
  auto graph   = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  ASSERT_FALSE(actions->empty());
  // First action should be compile; inputs should contain the source file
  const auto& first = (*actions)[0];
  EXPECT_EQ(first.kind, BuildAction::Kind::Compile);
  EXPECT_EQ(first.inputs[0], "pkg/lib.cpp");
}

// ─────────────────────────────────────────────────────────────────────────────
// CppAdapter – compute_cache_key (determinism)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CppAdapterCacheKeyTest, SameInputsProduceSameKey) {
  auto adapter = make_cpp_adapter();
  BuildNode node1("//pkg:lib", "cc_library", {"pkg/lib.cpp"}, {}, {});
  BuildNode node2("//pkg:lib", "cc_library", {"pkg/lib.cpp"}, {}, {});

  auto t1 = adapter.parse_target(node1).value();
  auto t2 = adapter.parse_target(node2).value();

  auto k1 = adapter.compute_cache_key(t1);
  auto k2 = adapter.compute_cache_key(t2);

  ASSERT_TRUE(k1.has_value());
  ASSERT_TRUE(k2.has_value());
  EXPECT_EQ(*k1, *k2);
}

TEST(CppAdapterCacheKeyTest, DifferentTargetLabelsProduceDifferentKeys) {
  auto adapter = make_cpp_adapter();
  BuildNode node1("//pkg:lib_a", "cc_library", {"pkg/lib.cpp"}, {}, {});
  BuildNode node2("//pkg:lib_b", "cc_library", {"pkg/lib.cpp"}, {}, {});

  auto t1 = adapter.parse_target(node1).value();
  auto t2 = adapter.parse_target(node2).value();

  auto k1 = adapter.compute_cache_key(t1).value();
  auto k2 = adapter.compute_cache_key(t2).value();

  EXPECT_NE(k1, k2);
}

TEST(CppAdapterCacheKeyTest, DifferentSrcsProduceDifferentKeys) {
  auto adapter = make_cpp_adapter();
  BuildNode node1("//pkg:lib", "cc_library", {"pkg/lib_a.cpp"}, {}, {});
  BuildNode node2("//pkg:lib", "cc_library", {"pkg/lib_b.cpp"}, {}, {});

  auto t1 = adapter.parse_target(node1).value();
  auto t2 = adapter.parse_target(node2).value();

  auto k1 = adapter.compute_cache_key(t1).value();
  auto k2 = adapter.compute_cache_key(t2).value();

  EXPECT_NE(k1, k2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AdapterRegistry
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterRegistryTest, InitiallyEmpty) {
  AdapterRegistry registry;
  EXPECT_EQ(registry.size(), 0u);
}

TEST(AdapterRegistryTest, RegisterAndFindByName) {
  AdapterRegistry registry;
  registry.register_adapter(std::make_unique<CppAdapter>(
      CppToolchain{"g++", "ar", "gcc", "13.0"}));

  EXPECT_EQ(registry.size(), 1u);
  const auto* adapter = registry.find_by_name("cpp");
  ASSERT_NE(adapter, nullptr);
  EXPECT_EQ(adapter->info().name, "cpp");
}

TEST(AdapterRegistryTest, FindByNameReturnsNullForUnknown) {
  AdapterRegistry registry;
  EXPECT_EQ(registry.find_by_name("rust"), nullptr);
}

TEST(AdapterRegistryTest, FindForKindReturnsCorrectAdapter) {
  AdapterRegistry registry;
  registry.register_adapter(std::make_unique<CppAdapter>(
      CppToolchain{"g++", "ar", "gcc", "13.0"}));

  const auto* cpp_adapter = registry.find_for_kind("cc_library");
  ASSERT_NE(cpp_adapter, nullptr);
  EXPECT_EQ(cpp_adapter->info().name, "cpp");
}

TEST(AdapterRegistryTest, FindForKindReturnsNullForUnsupportedKind) {
  AdapterRegistry registry;
  registry.register_adapter(std::make_unique<CppAdapter>(
      CppToolchain{"g++", "ar", "gcc", "13.0"}));

  EXPECT_EQ(registry.find_for_kind("java_library"), nullptr);
}

TEST(AdapterRegistryTest, AllAdaptersReturnsAll) {
  AdapterRegistry registry;
  registry.register_adapter(std::make_unique<CppAdapter>(
      CppToolchain{"g++", "ar", "gcc", "13.0"}));

  auto all = registry.all_adapters();
  EXPECT_EQ(all.size(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Diagnostic helpers
// ─────────────────────────────────────────────────────────────────────────────

TEST(DiagnosticTest, LevelStringValues) {
  Diagnostic info{Diagnostic::Level::Info, "msg", std::nullopt};
  EXPECT_EQ(info.level_string(), "INFO");

  Diagnostic warn{Diagnostic::Level::Warning, "msg", std::nullopt};
  EXPECT_EQ(warn.level_string(), "WARNING");

  Diagnostic err{Diagnostic::Level::Error, "msg", std::nullopt};
  EXPECT_EQ(err.level_string(), "ERROR");
}

TEST(DiagnosticTest, UnsupportedKindPopulatesDiagnostics) {
  auto adapter = make_cpp_adapter();
  BuildNode node("//pkg:jar", "java_library", {}, {}, {});

  auto result = adapter.parse_target(node);
  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(adapter.diagnostics().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// BuildAction kind_string
// ─────────────────────────────────────────────────────────────────────────────

TEST(BuildActionTest, KindStringValues) {
  BuildAction compile_action;
  compile_action.kind = BuildAction::Kind::Compile;
  EXPECT_EQ(compile_action.kind_string(), "compile");

  BuildAction archive_action;
  archive_action.kind = BuildAction::Kind::Archive;
  EXPECT_EQ(archive_action.kind_string(), "archive");

  BuildAction link_action;
  link_action.kind = BuildAction::Kind::Link;
  EXPECT_EQ(link_action.kind_string(), "link");

  BuildAction test_action;
  test_action.kind = BuildAction::Kind::Test;
  EXPECT_EQ(test_action.kind_string(), "test");

  BuildAction custom_action;
  custom_action.kind = BuildAction::Kind::Custom;
  EXPECT_EQ(custom_action.kind_string(), "custom");
}

} // namespace horcrux::core::test
