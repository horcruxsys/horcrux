// Horcrux - Graph Resolution Regression Tests
// Copyright (C) 2026 Horcrux Project Contributors
// Licensed under the MIT License
//
// Release-critical regression suite: verifies that the build graph correctly
// resolves dependencies, detects cycles, and produces consistent topological
// orderings.  These tests guard against regressions on the critical path.

#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/build_edge.h"
#include "../src/core/build_graph.h"
#include "../src/core/build_node.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static auto make_node(std::string label, std::string kind = "cc_library") -> BuildNode {
  return BuildNode{std::move(label), std::move(kind), {}, {}, {}};
}

/// Build a linear chain: lib0 ← lib1 ← … ← lib(depth-1)
static auto make_chain(int depth) -> BuildGraph {
  auto b = BuildGraph::builder();
  for (int i = 0; i < depth; ++i) {
    b.add_node(make_node("//pkg:lib" + std::to_string(i)));
  }
  for (int i = 1; i < depth; ++i) {
    b.add_edge(BuildEdge{"//pkg:lib" + std::to_string(i), "//pkg:lib" + std::to_string(i - 1)});
  }
  return b.build().value();
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: transitive dependency resolution is complete
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphRegressionTest, TransitiveDepsIncludeAllAncestors) {
  // lib0 ← lib1 ← lib2 ← lib3
  auto g = make_chain(4);

  auto deps = g.get_transitive_dependencies("//pkg:lib3");
  ASSERT_TRUE(deps.has_value());

  std::unordered_set<std::string> dep_set(deps->begin(), deps->end());
  EXPECT_TRUE(dep_set.count("//pkg:lib2"));
  EXPECT_TRUE(dep_set.count("//pkg:lib1"));
  EXPECT_TRUE(dep_set.count("//pkg:lib0"));
  EXPECT_FALSE(dep_set.count("//pkg:lib3")); // self not included
}

TEST(GraphRegressionTest, TransitiveDepsOnLeafNodeIsEmpty) {
  auto b = BuildGraph::builder();
  b.add_node(make_node("//pkg:leaf"));
  auto g = b.build().value();

  auto deps = g.get_transitive_dependencies("//pkg:leaf");
  ASSERT_TRUE(deps.has_value());
  EXPECT_TRUE(deps->empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: direct dependency resolution is correct
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphRegressionTest, DirectDepsReturnImmediateDependenciesOnly) {
  // app → libA, app → libB, libA → libC
  auto b = BuildGraph::builder();
  for (auto& lbl : {"//pkg:app", "//pkg:libA", "//pkg:libB", "//pkg:libC"}) {
    b.add_node(make_node(lbl));
  }
  b.add_edge(BuildEdge{"//pkg:app", "//pkg:libA"});
  b.add_edge(BuildEdge{"//pkg:app", "//pkg:libB"});
  b.add_edge(BuildEdge{"//pkg:libA", "//pkg:libC"});
  auto g = b.build().value();

  auto direct = g.get_dependencies("//pkg:app");
  ASSERT_TRUE(direct.has_value());

  std::unordered_set<std::string> dep_set(direct->begin(), direct->end());
  EXPECT_TRUE(dep_set.count("//pkg:libA"));
  EXPECT_TRUE(dep_set.count("//pkg:libB"));
  EXPECT_FALSE(dep_set.count("//pkg:libC")); // transitive — must not appear
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: cycle detection prevents invalid graphs
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphRegressionTest, DirectCycleIsDetected) {
  auto b = BuildGraph::builder();
  b.add_node(make_node("//pkg:a"));
  b.add_node(make_node("//pkg:b"));
  b.add_edge(BuildEdge{"//pkg:a", "//pkg:b"});

  auto result = b.add_edge(BuildEdge{"//pkg:b", "//pkg:a"});
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), GraphError::CycleDetected);
}

TEST(GraphRegressionTest, IndirectCycleIsDetected) {
  auto b = BuildGraph::builder();
  for (auto& l : {"//pkg:a", "//pkg:b", "//pkg:c"}) {
    b.add_node(make_node(l));
  }
  b.add_edge(BuildEdge{"//pkg:a", "//pkg:b"});
  b.add_edge(BuildEdge{"//pkg:b", "//pkg:c"});

  auto result = b.add_edge(BuildEdge{"//pkg:c", "//pkg:a"});
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), GraphError::CycleDetected);
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: topological order satisfies all dependency constraints
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphRegressionTest, TopologicalOrderRespectsDependencies) {
  // app → libA → libB
  auto b = BuildGraph::builder();
  for (auto& l : {"//pkg:app", "//pkg:libA", "//pkg:libB"}) {
    b.add_node(make_node(l));
  }
  b.add_edge(BuildEdge{"//pkg:app", "//pkg:libA"});
  b.add_edge(BuildEdge{"//pkg:libA", "//pkg:libB"});
  auto g = b.build().value();

  auto order = g.topological_sort();

  auto pos = [&](const std::string& label) {
    for (size_t i = 0; i < order.size(); ++i) {
      if (order[i] == label) {
        return static_cast<int>(i);
      }
    }
    return -1;
  };

  EXPECT_LT(pos("//pkg:libB"), pos("//pkg:libA"));
  EXPECT_LT(pos("//pkg:libA"), pos("//pkg:app"));
}

TEST(GraphRegressionTest, TopologicalOrderIsDeterministic) {
  auto make = []() {
    auto b = BuildGraph::builder();
    for (auto& l : {"//pkg:a", "//pkg:b", "//pkg:c"}) {
      b.add_node(make_node(l));
    }
    b.add_edge(BuildEdge{"//pkg:a", "//pkg:b"});
    b.add_edge(BuildEdge{"//pkg:b", "//pkg:c"});
    return b.build().value();
  };

  auto g1 = make();
  auto g2 = make();
  EXPECT_EQ(g1.topological_sort(), g2.topological_sort());
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: reverse dependency resolution
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphRegressionTest, ReverseDepsIdentifyAllDependents) {
  // app → lib, tool → lib
  auto b = BuildGraph::builder();
  for (auto& l : {"//pkg:app", "//pkg:tool", "//pkg:lib"}) {
    b.add_node(make_node(l));
  }
  b.add_edge(BuildEdge{"//pkg:app", "//pkg:lib"});
  b.add_edge(BuildEdge{"//pkg:tool", "//pkg:lib"});
  auto g = b.build().value();

  auto rdeps = g.get_dependents("//pkg:lib");
  ASSERT_TRUE(rdeps.has_value());

  std::unordered_set<std::string> rdep_set(rdeps->begin(), rdeps->end());
  EXPECT_TRUE(rdep_set.count("//pkg:app"));
  EXPECT_TRUE(rdep_set.count("//pkg:tool"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: node hash is stable across identical constructions
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphRegressionTest, NodeHashIsStable) {
  BuildNode n1{"//src:lib", "cc_library", {"a.cpp", "b.cpp"}, {"lib.a"}, {{"opt", "O2"}}};
  BuildNode n2{"//src:lib", "cc_library", {"a.cpp", "b.cpp"}, {"lib.a"}, {{"opt", "O2"}}};
  EXPECT_EQ(n1.compute_hash(), n2.compute_hash());
}

TEST(GraphRegressionTest, NodeHashChangesWithDifferentInputs) {
  BuildNode n1{"//src:lib", "cc_library", {"a.cpp"}, {}, {}};
  BuildNode n2{"//src:lib", "cc_library", {"b.cpp"}, {}, {}};
  EXPECT_NE(n1.compute_hash(), n2.compute_hash());
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: adding a node that already exists fails
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphRegressionTest, DuplicateNodeAdditionReturnsError) {
  auto b = BuildGraph::builder();
  b.add_node(make_node("//pkg:a"));
  auto result = b.add_node(make_node("//pkg:a"));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), GraphError::NodeAlreadyExists);
}

// ─────────────────────────────────────────────────────────────────────────────
// Regression: large graph resolution is correct and terminating
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphRegressionTest, LargeLinearChainResolvesCorrectly) {
  constexpr int kDepth = 200;
  auto g = make_chain(kDepth);

  auto root = "//pkg:lib" + std::to_string(kDepth - 1);
  auto deps = g.get_transitive_dependencies(root);
  ASSERT_TRUE(deps.has_value());
  EXPECT_EQ(deps->size(), static_cast<size_t>(kDepth - 1));
}

TEST(GraphRegressionTest, DiamondDependencyResolvedWithoutDuplicates) {
  //   top
  //  /   \
  // left right
  //  \   /
  //   base
  auto b = BuildGraph::builder();
  for (auto& l : {"//pkg:top", "//pkg:left", "//pkg:right", "//pkg:base"}) {
    b.add_node(make_node(l));
  }
  b.add_edge(BuildEdge{"//pkg:top", "//pkg:left"});
  b.add_edge(BuildEdge{"//pkg:top", "//pkg:right"});
  b.add_edge(BuildEdge{"//pkg:left", "//pkg:base"});
  b.add_edge(BuildEdge{"//pkg:right", "//pkg:base"});
  auto g = b.build().value();

  auto deps = g.get_transitive_dependencies("//pkg:top");
  ASSERT_TRUE(deps.has_value());

  // base must appear exactly once
  int base_count = 0;
  for (const auto& d : *deps) {
    if (d == "//pkg:base") {
      ++base_count;
    }
  }
  EXPECT_EQ(base_count, 1);
}

} // namespace horcrux::core::test
