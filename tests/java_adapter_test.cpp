// Horcrux - Java Adapter Unit Tests (non-Android)
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/adapter.h"
#include "../src/core/adapter_registry.h"
#include "../src/core/build_graph.h"
#include "../src/core/build_node.h"
#include "../src/core/java_adapter.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static auto make_java_adapter() -> JavaAdapter {
  JavaToolchain tc;
  tc.javac = "javac";
  tc.java = "java";
  tc.jar_tool = "jar";
  tc.java_version = "17.0.0";
  tc.source_version = "11";
  tc.target_version = "11";
  return JavaAdapter{tc};
}

static auto build_empty_graph() -> BuildGraph {
  auto builder = BuildGraph::builder();
  return builder.build().value();
}

// ─────────────────────────────────────────────────────────────────────────────
// Identity and capability
// ─────────────────────────────────────────────────────────────────────────────

TEST(JavaAdapterInfoTest, NameIsJava) {
  auto adapter = make_java_adapter();
  EXPECT_EQ(adapter.info().name, "java");
}

TEST(JavaAdapterInfoTest, VersionIsPresent) {
  auto adapter = make_java_adapter();
  EXPECT_FALSE(adapter.info().version.empty());
}

TEST(JavaAdapterInfoTest, SupportedKinds) {
  auto adapter = make_java_adapter();
  const auto& kinds = adapter.info().supported_kinds;
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "java_library"), kinds.end());
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "java_binary"), kinds.end());
  EXPECT_NE(std::find(kinds.begin(), kinds.end(), "java_test"), kinds.end());
}

TEST(JavaAdapterInfoTest, SupportsKindHelper) {
  auto adapter = make_java_adapter();
  EXPECT_TRUE(adapter.supports_kind("java_library"));
  EXPECT_TRUE(adapter.supports_kind("java_binary"));
  EXPECT_TRUE(adapter.supports_kind("java_test"));
  EXPECT_FALSE(adapter.supports_kind("cc_library"));
  EXPECT_FALSE(adapter.supports_kind("py_binary"));
  EXPECT_FALSE(adapter.supports_kind("rust_library"));
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_target
// ─────────────────────────────────────────────────────────────────────────────

TEST(JavaAdapterParseTest, ParsesLibraryNode) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:mylib", "java_library", {"pkg/MyLib.java"}, {"pkg/mylib.jar"}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->label, "//pkg:mylib");
  EXPECT_EQ(result->kind, "java_library");
}

TEST(JavaAdapterParseTest, ParsesBinaryNode) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:mybin", "java_binary", {"pkg/Main.java"}, {"pkg/mybin.jar"},
                 {{"main_class", "com.example.Main"}});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->kind, "java_binary");
  EXPECT_EQ(result->attrs.at("main_class")[0], "com.example.Main");
}

TEST(JavaAdapterParseTest, ParsesTestNode) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:mytest", "java_test", {"pkg/MyTest.java"}, {}, {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->kind, "java_test");
}

TEST(JavaAdapterParseTest, RejectsUnsupportedKind) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:mylib", "cc_library", {}, {}, {});

  auto result = adapter.parse_target(node);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), AdapterError::UnsupportedKind);
  EXPECT_FALSE(adapter.diagnostics().empty());
}

TEST(JavaAdapterParseTest, FilterNonJavaInputs) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:lib", "java_library", {"pkg/MyLib.java", "pkg/README.md", "pkg/lib.so"}, {},
                 {});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  const auto& srcs = result->attrs.at("srcs");
  EXPECT_EQ(srcs.size(), 1u);
  EXPECT_EQ(srcs[0], "pkg/MyLib.java");
}

TEST(JavaAdapterParseTest, ParsesJavacOpts) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:lib", "java_library", {"pkg/MyLib.java"}, {},
                 {{"javacopts", "-Xlint:all -Werror"}});

  auto result = adapter.parse_target(node);
  ASSERT_TRUE(result.has_value());
  const auto& opts = result->attrs.at("javacopts");
  EXPECT_EQ(opts.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// plan_actions
// ─────────────────────────────────────────────────────────────────────────────

TEST(JavaAdapterPlanTest, LibraryPlanHasCompileAndJarActions) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:mylib", "java_library", {"pkg/MyLib.java"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  EXPECT_GE(actions->size(), 2u);

  bool has_compile = false;
  bool has_archive = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Compile) {
      has_compile = true;
    }
    if (a.kind == BuildAction::Kind::Archive) {
      has_archive = true;
    }
  }
  EXPECT_TRUE(has_compile);
  EXPECT_TRUE(has_archive);
}

TEST(JavaAdapterPlanTest, TestPlanHasCompileJarAndTestActions) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:mytest", "java_test", {"pkg/MyTest.java"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  bool has_compile = false;
  bool has_archive = false;
  bool has_test = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Compile) {
      has_compile = true;
    }
    if (a.kind == BuildAction::Kind::Archive) {
      has_archive = true;
    }
    if (a.kind == BuildAction::Kind::Test) {
      has_test = true;
    }
  }
  EXPECT_TRUE(has_compile);
  EXPECT_TRUE(has_archive);
  EXPECT_TRUE(has_test);
}

TEST(JavaAdapterPlanTest, NoSourcesProducesNoActions) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:nolib", "java_library", {}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  EXPECT_EQ(actions->size(), 0u);
}

TEST(JavaAdapterPlanTest, CompileActionIncludesSourceFile) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:lib", "java_library", {"pkg/MyLib.java"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  ASSERT_FALSE(actions->empty());

  const auto& first = (*actions)[0];
  EXPECT_EQ(first.kind, BuildAction::Kind::Compile);
  // Source file should appear in command
  bool found_source = false;
  for (const auto& arg : first.command) {
    if (arg == "pkg/MyLib.java") {
      found_source = true;
    }
  }
  EXPECT_TRUE(found_source);
}

TEST(JavaAdapterPlanTest, JarActionOutputHasJarExtension) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:mylib", "java_library", {"pkg/MyLib.java"}, {}, {});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  bool found_jar_output = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Archive) {
      for (const auto& out : a.outputs) {
        if (out.find(".jar") != std::string::npos) {
          found_jar_output = true;
        }
      }
    }
  }
  EXPECT_TRUE(found_jar_output);
}

TEST(JavaAdapterPlanTest, BinaryJarIncludesMainClassEntry) {
  auto adapter = make_java_adapter();
  BuildNode node("//pkg:mybin", "java_binary", {"pkg/Main.java"}, {},
                 {{"main_class", "com.example.Main"}});

  auto target = adapter.parse_target(node).value();
  auto graph = build_empty_graph();
  auto actions = adapter.plan_actions(target, graph, "/tmp/out");

  ASSERT_TRUE(actions.has_value());
  bool found_main_class = false;
  for (const auto& a : *actions) {
    if (a.kind == BuildAction::Kind::Archive) {
      for (const auto& arg : a.command) {
        if (arg == "com.example.Main") {
          found_main_class = true;
        }
      }
    }
  }
  EXPECT_TRUE(found_main_class);
}

// ─────────────────────────────────────────────────────────────────────────────
// compute_cache_key
// ─────────────────────────────────────────────────────────────────────────────

TEST(JavaAdapterCacheKeyTest, SameInputsProduceSameKey) {
  auto adapter = make_java_adapter();
  BuildNode n1("//pkg:lib", "java_library", {"pkg/MyLib.java"}, {}, {});
  BuildNode n2("//pkg:lib", "java_library", {"pkg/MyLib.java"}, {}, {});

  auto t1 = adapter.parse_target(n1).value();
  auto t2 = adapter.parse_target(n2).value();

  auto k1 = adapter.compute_cache_key(t1);
  auto k2 = adapter.compute_cache_key(t2);

  ASSERT_TRUE(k1.has_value());
  ASSERT_TRUE(k2.has_value());
  EXPECT_EQ(*k1, *k2);
}

TEST(JavaAdapterCacheKeyTest, DifferentLabelsProduceDifferentKeys) {
  auto adapter = make_java_adapter();
  BuildNode n1("//pkg:lib_a", "java_library", {"pkg/MyLib.java"}, {}, {});
  BuildNode n2("//pkg:lib_b", "java_library", {"pkg/MyLib.java"}, {}, {});

  auto k1 = adapter.compute_cache_key(adapter.parse_target(n1).value()).value();
  auto k2 = adapter.compute_cache_key(adapter.parse_target(n2).value()).value();

  EXPECT_NE(k1, k2);
}

TEST(JavaAdapterCacheKeyTest, DifferentSrcsProduceDifferentKeys) {
  auto adapter = make_java_adapter();
  BuildNode n1("//pkg:lib", "java_library", {"pkg/A.java"}, {}, {});
  BuildNode n2("//pkg:lib", "java_library", {"pkg/B.java"}, {}, {});

  auto k1 = adapter.compute_cache_key(adapter.parse_target(n1).value()).value();
  auto k2 = adapter.compute_cache_key(adapter.parse_target(n2).value()).value();

  EXPECT_NE(k1, k2);
}

TEST(JavaAdapterCacheKeyTest, DifferentJavacoptsProduceDifferentKeys) {
  auto adapter = make_java_adapter();
  BuildNode n1("//pkg:lib", "java_library", {"pkg/A.java"}, {}, {{"javacopts", "-Xlint:all"}});
  BuildNode n2("//pkg:lib", "java_library", {"pkg/A.java"}, {}, {{"javacopts", "-Xlint:none"}});

  auto k1 = adapter.compute_cache_key(adapter.parse_target(n1).value()).value();
  auto k2 = adapter.compute_cache_key(adapter.parse_target(n2).value()).value();

  EXPECT_NE(k1, k2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AdapterRegistry integration
// ─────────────────────────────────────────────────────────────────────────────

TEST(JavaAdapterRegistryTest, RegisterAndFindByName) {
  AdapterRegistry registry;
  registry.register_adapter(
      std::make_unique<JavaAdapter>(JavaToolchain{"javac", "java", "jar", "17.0.0", "11", "11"}));

  EXPECT_EQ(registry.size(), 1u);
  EXPECT_NE(registry.find_by_name("java"), nullptr);
}

TEST(JavaAdapterRegistryTest, FindForKindJavaLibrary) {
  AdapterRegistry registry;
  registry.register_adapter(
      std::make_unique<JavaAdapter>(JavaToolchain{"javac", "java", "jar", "17.0.0", "11", "11"}));

  EXPECT_NE(registry.find_for_kind("java_library"), nullptr);
  EXPECT_NE(registry.find_for_kind("java_binary"), nullptr);
  EXPECT_NE(registry.find_for_kind("java_test"), nullptr);
  EXPECT_EQ(registry.find_for_kind("py_library"), nullptr);
}

} // namespace horcrux::core::test
