// Horcrux - Rust Adapter Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/adapter.h"
#include "../src/core/adapter_registry.h"
#include "../src/core/build_graph.h"
#include "../src/core/build_node.h"
#include "../src/core/rust_adapter.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static auto make_rust_adapter() -> RustAdapter {
  RustToolchain tc;
  tc.rustc = "rustc";
  tc.cargo = "cargo";
  tc.archiver = "ar";
  tc.rustc_version = "1.76.0";
  tc.edition = "2021";
  return RustAdapter{tc};
}

static auto build_empty_graph() -> BuildGraph {
  auto builder = BuildGraph::builder();
  return builder.build().value();
}

// ─────────────────────────────────────────────────────────────────────────────
// Identity and capability
// ─────────────────────────────────────────────────────────────────────────────

TEST(RustAdapterInfoTest, NameIsRust) {
  auto adapter = make_rust_adapter();
  EXPECT_EQ(adapter.info().name, "rust");
}

TEST(RustAdapterInfoTest, VersionIsPresent) {
  auto adapter = make_rust_adapter();
  EXPECT_FALSE(adapter.info().version.empty());
}

TEST(RustAdapterInfoTest, SupportedKinds) {
  auto adapter = make_rust_adapter();
  const auto& kinds = adapter.info().supported_kinds;
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "rust_library"), kinds.end());
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "rust_binary"), kinds.end());
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "rust_test"), kinds.end());
}

TEST(RustAdapterInfoTest, SupportsKindHelper) {
  auto adapter = make_rust_adapter();
  EXPECT_TRUE(adapter.supports_kind("rust_library"));
  EXPECT_TRUE(adapter.supports_kind("rust_binary"));
  EXPECT_TRUE(adapter.supports_kind("rust_test"));
  EXPECT_FALSE(adapter.supports_kind("cc_library"));
  EXPECT_FALSE(adapter.supports_kind("py_binary"));
  EXPECT_FALSE(adapter.supports_kind("java_library"));
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_target
// ─────────────────────────────────────────────────────────────────────────────

TEST(RustAdapterParseTest, ParsesLibraryNode) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:mylib", "rust_library", {"pkg/lib.rs"}, {"pkg/libmylib.rlib"}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->label, "//pkg:mylib");
  EXPECT_EQ(result->kind, "rust_library");
}

TEST(RustAdapterParseTest, ParsesBinaryNode) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:mybin", "rust_binary", {"pkg/main.rs"}, {"pkg/mybin"}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->kind, "rust_binary");
}

TEST(RustAdapterParseTest, ParsesTestNode) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:mytest", "rust_test", {"pkg/test_main.rs"}, {}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->kind, "rust_test");
}

TEST(RustAdapterParseTest, RejectsUnsupportedKind) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:mylib", "cc_library", {}, {}, {});

  auto result = adapter.parse_target(node);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AdapterError::UnsupportedKind);
  EXPECT_FALSE(adapter.diagnostics().empty());
}

TEST(RustAdapterParseTest, FilterNonRustInputs) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:lib", "rust_library", {"pkg/lib.rs", "pkg/README.md", "pkg/lib.h"}, {}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  const auto& srcs = result->attrs.at("srcs");
  EXPECT_EQ(srcs.size(), 1u);
  EXPECT_EQ(srcs[0], "pkg/lib.rs");
}

TEST(RustAdapterParseTest, EditionDefaultsToToolchainEdition) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:lib", "rust_library", {"pkg/lib.rs"}, {}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  const auto& edition = result->attrs.at("edition");
  EXPECT_EQ(edition[0], "2021");
}

TEST(RustAdapterParseTest, EditionFromAttribute) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:lib", "rust_library", {"pkg/lib.rs"}, {}, {{"edition", "2018"}});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->attrs.at("edition")[0], "2018");
}

// ─────────────────────────────────────────────────────────────────────────────
// plan_actions
// ─────────────────────────────────────────────────────────────────────────────

TEST(RustAdapterPlanTest, LibraryPlanHasCompileAction) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:mylib", "rust_library", {"pkg/lib.rs"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  EXPECT_GE(actions->size(), 1u);

  bool has_compile = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Compile) {
      has_compile = true;
    }
  }
  EXPECT_TRUE(has_compile);
}

TEST(RustAdapterPlanTest, BinaryPlanHasCompileAction) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:mybin", "rust_binary", {"pkg/main.rs"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  bool has_compile = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Compile) {
      has_compile = true;
    }
  }
  EXPECT_TRUE(has_compile);
}

TEST(RustAdapterPlanTest, TestPlanHasCompileAndTestActions) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:mytest", "rust_test", {"pkg/test.rs"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  bool has_compile = false;
  bool has_test = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Compile) {
      has_compile = true;
    }
    if (a.kind == BuildAction::Kind::Test) {
      has_test = true;
    }
  }
  EXPECT_TRUE(has_compile);
  EXPECT_TRUE(has_test);
}

TEST(RustAdapterPlanTest, NoSourcesProducesNoActions) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:nolib", "rust_library", {}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  EXPECT_EQ(actions->size(), 0u);
}

TEST(RustAdapterPlanTest, CompileActionIncludesSourceFile) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:lib", "rust_library", {"pkg/lib.rs"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  ASSERT_FALSE(actions->empty());

  const auto& first = (*actions)[0];
  EXPECT_EQ(first.kind, BuildAction::Kind::Compile);
  EXPECT_EQ(first.inputs[0], "pkg/lib.rs");
}

TEST(RustAdapterPlanTest, LibraryOutputIsRlib) {
  auto adapter = make_rust_adapter();
  BuildNode node("//pkg:mylib", "rust_library", {"pkg/lib.rs"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  ASSERT_FALSE(actions->empty());
  const auto& compile = (*actions)[0];
  ASSERT_FALSE(compile.outputs.empty());
  EXPECT_NE(compile.outputs[0].find(".rlib"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// compute_cache_key
// ─────────────────────────────────────────────────────────────────────────────

TEST(RustAdapterCacheKeyTest, SameInputsProduceSameKey) {
  auto adapter = make_rust_adapter();
  BuildNode n1("//pkg:lib", "rust_library", {"pkg/lib.rs"}, {}, {});
  BuildNode n2("//pkg:lib", "rust_library", {"pkg/lib.rs"}, {}, {});

  auto t1 = adapter.parse_target(n1).value();
  auto t2 = adapter.parse_target(n2).value();

  auto k1 = adapter.compute_cache_key(t1);
  auto k2 = adapter.compute_cache_key(t2);

  ASSERT_TRUE(k1.has_value());
  ASSERT_TRUE(k2.has_value());
  EXPECT_EQ(*k1, *k2);
}

TEST(RustAdapterCacheKeyTest, DifferentLabelsProduceDifferentKeys) {
  auto adapter = make_rust_adapter();
  BuildNode n1("//pkg:lib_a", "rust_library", {"pkg/lib.rs"}, {}, {});
  BuildNode n2("//pkg:lib_b", "rust_library", {"pkg/lib.rs"}, {}, {});

  auto k1 = adapter.compute_cache_key(adapter.parse_target(n1).value()).value();
  auto k2 = adapter.compute_cache_key(adapter.parse_target(n2).value()).value();

  EXPECT_NE(k1, k2);
}

TEST(RustAdapterCacheKeyTest, DifferentSrcsProduceDifferentKeys) {
  auto adapter = make_rust_adapter();
  BuildNode n1("//pkg:lib", "rust_library", {"pkg/lib_a.rs"}, {}, {});
  BuildNode n2("//pkg:lib", "rust_library", {"pkg/lib_b.rs"}, {}, {});

  auto k1 = adapter.compute_cache_key(adapter.parse_target(n1).value()).value();
  auto k2 = adapter.compute_cache_key(adapter.parse_target(n2).value()).value();

  EXPECT_NE(k1, k2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AdapterRegistry integration
// ─────────────────────────────────────────────────────────────────────────────

TEST(RustAdapterRegistryTest, RegisterAndFindByName) {
  AdapterRegistry registry;
  registry.register_adapter(
      std::make_unique<RustAdapter>(RustToolchain{"rustc", "cargo", "ar", "1.76.0", "2021"}));

  EXPECT_EQ(registry.size(), 1u);
  EXPECT_NE(registry.find_by_name("rust"), nullptr);
}

TEST(RustAdapterRegistryTest, FindForKindRustLibrary) {
  AdapterRegistry registry;
  registry.register_adapter(
      std::make_unique<RustAdapter>(RustToolchain{"rustc", "cargo", "ar", "1.76.0", "2021"}));

  EXPECT_NE(registry.find_for_kind("rust_library"), nullptr);
  EXPECT_NE(registry.find_for_kind("rust_binary"), nullptr);
  EXPECT_NE(registry.find_for_kind("rust_test"), nullptr);
  EXPECT_EQ(registry.find_for_kind("cc_library"), nullptr);
}

} // namespace horcrux::core::test
