// Horcrux - Cross-Adapter Conformance Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License
//
// These tests verify that all adapters honour the shared platform contracts:
//   1. Capability metadata is non-empty and consistent.
//   2. supports_kind() matches the supported_kinds list exactly.
//   3. parse_target() rejects a kind owned by another adapter.
//   4. compute_cache_key() is deterministic: same inputs → same key.
//   5. Different source inputs produce different cache keys.
//   6. Action plan is non-empty when sources are present.
//   7. Diagnostics are cleared between parse_target calls.

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
// Per-adapter fixture data
// ─────────────────────────────────────────────────────────────────────────────

struct AdapterConformanceCase {
  std::string name;
  std::string lib_kind;
  std::string bin_kind;
  std::string test_kind;
  std::string src_ext;      ///< primary source extension
  std::string foreign_kind; ///< a kind this adapter must reject
};

static const std::vector<AdapterConformanceCase> kCases = {
    {"cpp", "cc_library", "cc_binary", "cc_test", ".cpp", "py_library"},
    {"rust", "rust_library", "rust_binary", "rust_test", ".rs", "cc_library"},
    {"python", "py_library", "py_binary", "py_test", ".py", "java_library"},
    {"java", "java_library", "java_binary", "java_test", ".java", "rust_binary"},
};

/// Build an Adapter* for the given case name
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
  if (name == "java") {
    return std::make_unique<JavaAdapter>(
        JavaToolchain{"javac", "java", "jar", "17.0.0", "11", "11"});
  }
  return nullptr;
}

static auto build_empty_graph() -> BuildGraph {
  auto builder = BuildGraph::builder();
  return builder.build().value();
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 1 – AdapterInfo is populated
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterConformanceTest, InfoNameMatchesAdapterKind) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    ASSERT_NE(adapter, nullptr) << c.name;
    EXPECT_EQ(adapter->info().name, c.name) << c.name;
  }
}

TEST(AdapterConformanceTest, InfoVersionNonEmpty) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    ASSERT_NE(adapter, nullptr) << c.name;
    EXPECT_FALSE(adapter->info().version.empty()) << c.name;
  }
}

TEST(AdapterConformanceTest, InfoSupportedKindsNonEmpty) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    ASSERT_NE(adapter, nullptr) << c.name;
    EXPECT_FALSE(adapter->info().supported_kinds.empty()) << c.name;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 2 – supports_kind() is consistent with supported_kinds
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterConformanceTest, SupportsKindConsistentWithSupportedKindsList) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    ASSERT_NE(adapter, nullptr) << c.name;
    for (const auto& kind : adapter->info().supported_kinds) {
      EXPECT_TRUE(adapter->supports_kind(kind))
          << c.name << " does not support declared kind: " << kind;
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 3 – foreign kind is rejected
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterConformanceTest, ForeignKindIsRejected) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    ASSERT_NE(adapter, nullptr) << c.name;
    EXPECT_FALSE(adapter->supports_kind(c.foreign_kind))
        << c.name << " incorrectly claims to support " << c.foreign_kind;

    BuildNode foreign_node("//pkg:foreign", c.foreign_kind, {}, {}, {});
    auto result = adapter->parse_target(foreign_node);
    EXPECT_FALSE(result.has_value()) << c.name << " should reject " << c.foreign_kind;
    if (!result.has_value()) {
      EXPECT_EQ(result.error(), AdapterError::UnsupportedKind) << c.name;
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 4 – parse_target succeeds for each supported kind
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterConformanceTest, ParseTargetSucceedsForSupportedKinds) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    ASSERT_NE(adapter, nullptr) << c.name;

    for (const auto& kind : adapter->info().supported_kinds) {
      std::string src = "pkg/src" + c.src_ext;
      BuildNode node("//pkg:target", kind, {src}, {}, {});
      auto result = adapter->parse_target(node);
      EXPECT_TRUE(result.has_value()) << c.name << " failed to parse kind " << kind;
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 5 – cache key determinism
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterConformanceTest, CacheKeyIsDeterministic) {
  for (const auto& c : kCases) {
    auto adapter1 = make_adapter(c.name);
    auto adapter2 = make_adapter(c.name);
    ASSERT_NE(adapter1, nullptr) << c.name;
    ASSERT_NE(adapter2, nullptr) << c.name;

    std::string src = "pkg/src" + c.src_ext;
    BuildNode n1("//pkg:target", c.lib_kind, {src}, {}, {});
    BuildNode n2("//pkg:target", c.lib_kind, {src}, {}, {});

    auto t1 = adapter1->parse_target(n1);
    auto t2 = adapter2->parse_target(n2);
    ASSERT_TRUE(t1.has_value()) << c.name;
    ASSERT_TRUE(t2.has_value()) << c.name;

    auto k1 = adapter1->compute_cache_key(*t1);
    auto k2 = adapter2->compute_cache_key(*t2);

    ASSERT_TRUE(k1.has_value()) << c.name;
    ASSERT_TRUE(k2.has_value()) << c.name;
    EXPECT_EQ(*k1, *k2) << c.name << " cache key is not deterministic";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 6 – different sources produce different cache keys
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterConformanceTest, DifferentSrcsProduceDifferentCacheKeys) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    ASSERT_NE(adapter, nullptr) << c.name;

    std::string src_a = "pkg/src_a" + c.src_ext;
    std::string src_b = "pkg/src_b" + c.src_ext;

    BuildNode n1("//pkg:target", c.lib_kind, {src_a}, {}, {});
    BuildNode n2("//pkg:target", c.lib_kind, {src_b}, {}, {});

    auto t1 = adapter->parse_target(n1).value();
    auto t2 = adapter->parse_target(n2).value();

    auto k1 = adapter->compute_cache_key(t1).value();
    auto k2 = adapter->compute_cache_key(t2).value();

    EXPECT_NE(k1, k2) << c.name << " same key for different srcs";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 7 – plan_actions is non-empty when sources are present
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterConformanceTest, PlanActionsNonEmptyWhenSourcesPresent) {
  auto graph = build_empty_graph();

  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    ASSERT_NE(adapter, nullptr) << c.name;

    std::string src = "pkg/src" + c.src_ext;
    BuildNode node("//pkg:target", c.lib_kind, {src}, {}, {});
    auto target = adapter->parse_target(node).value();
    auto actions = adapter->plan_actions(target, graph, "/tmp/out");

    ASSERT_TRUE(actions.has_value()) << c.name << " plan_actions failed";
    EXPECT_FALSE(actions->empty()) << c.name << " plan_actions returned empty list with sources";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 8 – diagnostics are cleared between parse calls
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterConformanceTest, DiagnosticsClearedBetweenParseCalls) {
  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    ASSERT_NE(adapter, nullptr) << c.name;

    // First call: should produce an UnsupportedKind diagnostic
    BuildNode bad_node("//pkg:bad", c.foreign_kind, {}, {}, {});
    auto bad_result = adapter->parse_target(bad_node);
    EXPECT_FALSE(bad_result.has_value()) << c.name;
    EXPECT_FALSE(adapter->diagnostics().empty()) << c.name;

    // Second call: valid target — diagnostics should be cleared
    std::string src = "pkg/src" + c.src_ext;
    BuildNode good_node("//pkg:good", c.lib_kind, {src}, {}, {});
    auto good_result = adapter->parse_target(good_node);
    EXPECT_TRUE(good_result.has_value()) << c.name;
    EXPECT_TRUE(adapter->diagnostics().empty())
        << c.name << " diagnostics not cleared after successful parse";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 9 – AdapterRegistry multi-adapter coexistence
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterConformanceTest, AllAdaptersCoexistInRegistry) {
  AdapterRegistry registry;
  registry.register_adapter(
      std::make_unique<CppAdapter>(CppToolchain{"g++", "ar", "gcc", "13.0.0"}));
  registry.register_adapter(
      std::make_unique<RustAdapter>(RustToolchain{"rustc", "cargo", "ar", "1.76.0", "2021"}));
  registry.register_adapter(std::make_unique<PythonAdapter>(PythonToolchain{"python3", "3.11.0"}));
  registry.register_adapter(
      std::make_unique<JavaAdapter>(JavaToolchain{"javac", "java", "jar", "17.0.0", "11", "11"}));

  EXPECT_EQ(registry.size(), 4u);

  for (const auto& c : kCases) {
    EXPECT_NE(registry.find_by_name(c.name), nullptr) << c.name << " not found in registry";
    EXPECT_NE(registry.find_for_kind(c.lib_kind), nullptr) << c.lib_kind << " not routable";
    EXPECT_NE(registry.find_for_kind(c.bin_kind), nullptr) << c.bin_kind << " not routable";
    EXPECT_NE(registry.find_for_kind(c.test_kind), nullptr) << c.test_kind << " not routable";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Contract 10 – plan_actions produces no actions for empty source list
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdapterConformanceTest, PlanActionsEmptyForNoSources) {
  auto graph = build_empty_graph();

  for (const auto& c : kCases) {
    auto adapter = make_adapter(c.name);
    ASSERT_NE(adapter, nullptr) << c.name;

    BuildNode node("//pkg:nosrc", c.lib_kind, {}, {}, {});
    auto target = adapter->parse_target(node).value();
    auto actions = adapter->plan_actions(target, graph, "/tmp/out");

    ASSERT_TRUE(actions.has_value()) << c.name;
    EXPECT_EQ(actions->size(), 0u) << c.name << " produced actions for empty source list";
  }
}

} // namespace horcrux::core::test
