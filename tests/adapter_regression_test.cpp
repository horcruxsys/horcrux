// Horcrux - Adapter Conformance Regression Tests
// Copyright (C) 2026 Horcrux Project Contributors
// Licensed under the MIT License
//
// Release-critical regression suite: verifies that all language adapters
// honour the shared platform contracts after v2026.0401.0.  Any adapter that
// breaks these contracts is a release blocker.

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/adapter.h"
#include "../src/core/adapter_registry.h"
#include "../src/core/build_graph.h"
#include "../src/core/build_node.h"
#include "../src/core/cpp_adapter.h"
#include "../src/core/java_adapter.h"
#include "../src/core/python_adapter.h"
#include "../src/core/rust_adapter.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// Fixture data and factory
// ─────────────────────────────────────────────────────────────────────────────

struct AdapterCase {
  std::string name;
  std::string lib_kind;
  std::string bin_kind;
  std::string test_kind;
  std::string src_ext;
  std::string foreign_kind;
};

static const std::vector<AdapterCase> kCases = {
    {"cpp", "cc_library", "cc_binary", "cc_test", ".cpp", "py_library"},
    {"rust", "rust_library", "rust_binary", "rust_test", ".rs", "cc_library"},
    {"python", "py_library", "py_binary", "py_test", ".py", "java_library"},
    {"java", "java_library", "java_binary", "java_test", ".java", "rust_binary"},
};

static auto make_adapter(const std::string& name) -> std::unique_ptr<Adapter> {
  if (name == "cpp") {
    return std::make_unique<CppAdapter>(CppToolchain{"g++", "ar", "gcc", "13.0.0"});
  }
  if (name == "rust") {
    return std::make_unique<RustAdapter>(RustToolchain{"rustc", "cargo", "ar", "1.76.0", "2021"});
  }
  if (name == "python") {
    return std::make_unique<PythonAdapter>(PythonToolchain{"python3", "3.11.0"});
  }
  return std::make_unique<JavaAdapter>(JavaToolchain{"javac", "java", "jar", "17.0.0", "17", "17"});
}

static auto make_node(const std::string& label, const std::string& kind,
                      const std::vector<std::string>& srcs) -> BuildNode {
  return BuildNode{label, kind, srcs, {}, {}};
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 1: info() returns non-empty name and supported_kinds
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterRegressionTest, InfoIsNonEmpty) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    const auto& info = adapter->info();
    EXPECT_FALSE(info.name.empty()) << c.name;
    EXPECT_FALSE(info.supported_kinds.empty()) << c.name;
    EXPECT_FALSE(info.version.empty()) << c.name;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 2: supports_kind matches supported_kinds list
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterRegressionTest, SupportsKindMatchesSupportedKindsList) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    for (const auto& kind : adapter->info().supported_kinds) {
      EXPECT_TRUE(adapter->supports_kind(kind)) << c.name << " kind=" << kind;
    }
  }
}

TEST(AdapterRegressionTest, SupportsKindReturnsFalseForForeignKind) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    EXPECT_FALSE(adapter->supports_kind(c.foreign_kind)) << c.name;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 3: parse_target rejects a kind owned by another adapter
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterRegressionTest, ParseTargetRejectsForeignKind) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    BuildNode foreign_node = make_node("//pkg:foreign", c.foreign_kind, {"src" + c.src_ext});
    auto result = adapter->parse_target(foreign_node);
    EXPECT_FALSE(result.has_value()) << c.name << " should reject " << c.foreign_kind;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 4: compute_cache_key is deterministic (same inputs → same key)
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterRegressionTest, CacheKeyIsDeterministic) {
  for (const auto& c : kCases) {
    auto a1 = make_adapter(c.name);
    auto a2 = make_adapter(c.name);
    BuildNode node = make_node("//pkg:lib", c.lib_kind, {"src" + c.src_ext});

    auto t1 = a1->parse_target(node);
    auto t2 = a2->parse_target(node);
    ASSERT_TRUE(t1.has_value()) << c.name;
    ASSERT_TRUE(t2.has_value()) << c.name;

    auto k1 = a1->compute_cache_key(*t1);
    auto k2 = a2->compute_cache_key(*t2);
    ASSERT_TRUE(k1.has_value()) << c.name;
    ASSERT_TRUE(k2.has_value()) << c.name;
    EXPECT_EQ(*k1, *k2) << c.name << " cache key must be deterministic";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 5: different source inputs produce different cache keys
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterRegressionTest, DifferentSourcesProduceDifferentCacheKeys) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    BuildNode node_a = make_node("//pkg:lib", c.lib_kind, {"a" + c.src_ext});
    BuildNode node_b = make_node("//pkg:lib", c.lib_kind, {"b" + c.src_ext});

    auto t_a = adapter->parse_target(node_a);
    auto t_b = adapter->parse_target(node_b);
    ASSERT_TRUE(t_a.has_value()) << c.name;
    ASSERT_TRUE(t_b.has_value()) << c.name;

    auto k_a = adapter->compute_cache_key(*t_a);
    auto k_b = adapter->compute_cache_key(*t_b);
    ASSERT_TRUE(k_a.has_value()) << c.name;
    ASSERT_TRUE(k_b.has_value()) << c.name;
    EXPECT_NE(*k_a, *k_b) << c.name;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 6: plan_actions produces at least one action for a valid target
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterRegressionTest, PlanActionsProducesActionsForValidTarget) {
  auto empty_graph = BuildGraph::builder().build().value();
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    BuildNode node = make_node("//pkg:lib", c.lib_kind, {"src" + c.src_ext});

    auto target = adapter->parse_target(node);
    ASSERT_TRUE(target.has_value()) << c.name;

    auto actions = adapter->plan_actions(*target, empty_graph, "/tmp/out");
    ASSERT_TRUE(actions.has_value()) << c.name;
    EXPECT_FALSE(actions->empty()) << c.name << " must plan at least one action";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 7: diagnostics are cleared between parse_target calls
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterRegressionTest, DiagnosticsClearedBetweenParseCalls) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);

    // First call: a foreign kind injects diagnostics
    BuildNode foreign = make_node("//pkg:f", c.foreign_kind, {"src" + c.src_ext});
    adapter->parse_target(foreign);
    size_t diags_after_foreign = adapter->diagnostics().size();

    // Second call: a valid kind should clear prior diagnostics
    BuildNode valid = make_node("//pkg:v", c.lib_kind, {"src" + c.src_ext});
    auto result = adapter->parse_target(valid);
    ASSERT_TRUE(result.has_value()) << c.name;

    // After a successful parse the diagnostic count must not grow cumulatively
    EXPECT_LE(adapter->diagnostics().size(), diags_after_foreign)
        << c.name << ": diagnostics must not accumulate across calls";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 8: AdapterRegistry routes each kind to the correct adapter
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterRegressionTest, RegistryRoutesKindToCorrectAdapter) {
  AdapterRegistry registry;
  registry.register_adapter(make_adapter("cpp"));
  registry.register_adapter(make_adapter("rust"));
  registry.register_adapter(make_adapter("python"));
  registry.register_adapter(make_adapter("java"));

  for (const auto& c : kCases) {
    for (const auto& kind : {c.lib_kind, c.bin_kind, c.test_kind}) {
      auto found = registry.find_for_kind(kind);
      ASSERT_TRUE(found != nullptr) << "No adapter found for kind: " << kind;
      EXPECT_TRUE(found->supports_kind(kind)) << "Wrong adapter for kind: " << kind;
    }
  }
}

} // namespace horcrux::core::test
