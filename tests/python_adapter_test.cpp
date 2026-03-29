// Horcrux - Python Adapter Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/adapter.h"
#include "../src/core/adapter_registry.h"
#include "../src/core/build_graph.h"
#include "../src/core/build_node.h"
#include "../src/core/python_adapter.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static auto make_python_adapter() -> PythonAdapter {
  PythonToolchain tc;
  tc.python = "python3";
  tc.python_version = "3.11.0";
  return PythonAdapter{tc};
}

static auto build_empty_graph() -> BuildGraph {
  auto builder = BuildGraph::builder();
  return builder.build().value();
}

// ─────────────────────────────────────────────────────────────────────────────
// Identity and capability
// ─────────────────────────────────────────────────────────────────────────────

TEST(PythonAdapterInfoTest, NameIsPython) {
  auto adapter = make_python_adapter();
  EXPECT_EQ(adapter.info().name, "python");
}

TEST(PythonAdapterInfoTest, VersionIsPresent) {
  auto adapter = make_python_adapter();
  EXPECT_FALSE(adapter.info().version.empty());
}

TEST(PythonAdapterInfoTest, SupportedKinds) {
  auto adapter = make_python_adapter();
  const auto& kinds = adapter.info().supported_kinds;
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "py_library"), kinds.end());
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "py_binary"), kinds.end());
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "py_test"), kinds.end());
}

TEST(PythonAdapterInfoTest, SupportsKindHelper) {
  auto adapter = make_python_adapter();
  EXPECT_TRUE(adapter.supports_kind("py_library"));
  EXPECT_TRUE(adapter.supports_kind("py_binary"));
  EXPECT_TRUE(adapter.supports_kind("py_test"));
  EXPECT_FALSE(adapter.supports_kind("cc_library"));
  EXPECT_FALSE(adapter.supports_kind("rust_binary"));
  EXPECT_FALSE(adapter.supports_kind("java_library"));
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_target
// ─────────────────────────────────────────────────────────────────────────────

TEST(PythonAdapterParseTest, ParsesLibraryNode) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:mylib", "py_library", {"pkg/lib.py", "pkg/__init__.py"}, {}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->label, "//pkg:mylib");
  EXPECT_EQ(result->kind, "py_library");
}

TEST(PythonAdapterParseTest, ParsesBinaryNode) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:mybin", "py_binary", {"pkg/main.py"}, {}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->kind, "py_binary");
}

TEST(PythonAdapterParseTest, ParsesTestNode) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:mytest", "py_test", {"pkg/test_main.py"}, {}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->kind, "py_test");
}

TEST(PythonAdapterParseTest, RejectsUnsupportedKind) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:mylib", "cc_library", {}, {}, {});

  auto result = adapter.parse_target(node);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AdapterError::UnsupportedKind);
  EXPECT_FALSE(adapter.diagnostics().empty());
}

TEST(PythonAdapterParseTest, FilterNonPythonInputs) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:lib", "py_library", {"pkg/lib.py", "pkg/README.md", "pkg/config.yaml"}, {},
                 {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  const auto& srcs = result->attrs.at("srcs");
  EXPECT_EQ(srcs.size(), 1u);
  EXPECT_EQ(srcs[0], "pkg/lib.py");
}

TEST(PythonAdapterParseTest, MainModuleFromAttribute) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:mybin", "py_binary", {"pkg/main.py", "pkg/helper.py"}, {},
                 {{"main", "pkg/main.py"}});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->attrs.at("main")[0], "pkg/main.py");
}

TEST(PythonAdapterParseTest, MainModuleDefaultsToFirstSrc) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:mybin", "py_binary", {"pkg/main.py"}, {}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->attrs.at("main").empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// plan_actions
// ─────────────────────────────────────────────────────────────────────────────

TEST(PythonAdapterPlanTest, LibraryPlanHasPackageAction) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:mylib", "py_library", {"pkg/lib.py"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  EXPECT_GE(actions->size(), 1u);

  bool has_package = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Custom) {
      has_package = true;
    }
  }
  EXPECT_TRUE(has_package);
}

TEST(PythonAdapterPlanTest, TestPlanHasPackageAndTestActions) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:mytest", "py_test", {"pkg/test_lib.py"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  bool has_package = false;
  bool has_test = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Custom) {
      has_package = true;
    }
    if (a.kind == BuildAction::Kind::Test) {
      has_test = true;
    }
  }
  EXPECT_TRUE(has_package);
  EXPECT_TRUE(has_test);
}

TEST(PythonAdapterPlanTest, NoSourcesProducesNoActions) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:empty", "py_library", {}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  EXPECT_EQ(actions->size(), 0u);
}

TEST(PythonAdapterPlanTest, PackageActionIncludesSourceFiles) {
  auto adapter = make_python_adapter();
  BuildNode node("//pkg:lib", "py_library", {"pkg/lib.py", "pkg/utils.py"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  ASSERT_FALSE(actions->empty());

  const auto& first = (*actions)[0];
  EXPECT_FALSE(first.inputs.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// compute_cache_key
// ─────────────────────────────────────────────────────────────────────────────

TEST(PythonAdapterCacheKeyTest, SameInputsProduceSameKey) {
  auto adapter = make_python_adapter();
  BuildNode n1("//pkg:lib", "py_library", {"pkg/lib.py"}, {}, {});
  BuildNode n2("//pkg:lib", "py_library", {"pkg/lib.py"}, {}, {});

  auto t1 = adapter.parse_target(n1).value();
  auto t2 = adapter.parse_target(n2).value();

  auto k1 = adapter.compute_cache_key(t1);
  auto k2 = adapter.compute_cache_key(t2);

  ASSERT_TRUE(k1.has_value());
  ASSERT_TRUE(k2.has_value());
  EXPECT_EQ(*k1, *k2);
}

TEST(PythonAdapterCacheKeyTest, DifferentLabelsProduceDifferentKeys) {
  auto adapter = make_python_adapter();
  BuildNode n1("//pkg:lib_a", "py_library", {"pkg/lib.py"}, {}, {});
  BuildNode n2("//pkg:lib_b", "py_library", {"pkg/lib.py"}, {}, {});

  auto k1 = adapter.compute_cache_key(adapter.parse_target(n1).value()).value();
  auto k2 = adapter.compute_cache_key(adapter.parse_target(n2).value()).value();

  EXPECT_NE(k1, k2);
}

TEST(PythonAdapterCacheKeyTest, DifferentSrcsProduceDifferentKeys) {
  auto adapter = make_python_adapter();
  BuildNode n1("//pkg:lib", "py_library", {"pkg/lib_a.py"}, {}, {});
  BuildNode n2("//pkg:lib", "py_library", {"pkg/lib_b.py"}, {}, {});

  auto k1 = adapter.compute_cache_key(adapter.parse_target(n1).value()).value();
  auto k2 = adapter.compute_cache_key(adapter.parse_target(n2).value()).value();

  EXPECT_NE(k1, k2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AdapterRegistry integration
// ─────────────────────────────────────────────────────────────────────────────

TEST(PythonAdapterRegistryTest, RegisterAndFindByName) {
  AdapterRegistry registry;
  registry.register_adapter(std::make_unique<PythonAdapter>(PythonToolchain{"python3", "3.11.0"}));

  EXPECT_EQ(registry.size(), 1u);
  EXPECT_NE(registry.find_by_name("python"), nullptr);
}

TEST(PythonAdapterRegistryTest, FindForKindPyLibrary) {
  AdapterRegistry registry;
  registry.register_adapter(std::make_unique<PythonAdapter>(PythonToolchain{"python3", "3.11.0"}));

  EXPECT_NE(registry.find_for_kind("py_library"), nullptr);
  EXPECT_NE(registry.find_for_kind("py_binary"), nullptr);
  EXPECT_NE(registry.find_for_kind("py_test"), nullptr);
  EXPECT_EQ(registry.find_for_kind("cc_library"), nullptr);
}

} // namespace horcrux::core::test
