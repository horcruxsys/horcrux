// Horcrux - Build Graph Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "../src/core/build_graph.h"
#include "../src/core/build_node.h"
#include "../src/core/build_edge.h"

#include <gtest/gtest.h>

namespace horcrux::core::test {

// Test BuildNode creation and properties
TEST(BuildNodeTest, ConstructorInitializesProperties) {
    BuildNode node(
        "//src:test",
        "cc_library",
        {"file1.cpp", "file2.cpp"},
        {"libtest.a"},
        {{"visibility", "public"}}
    );

    EXPECT_EQ(node.label(), "//src:test");
    EXPECT_EQ(node.node_type(), "cc_library");
    EXPECT_EQ(node.inputs().size(), 2);
    EXPECT_EQ(node.outputs().size(), 1);
    EXPECT_EQ(node.inputs()[0], "file1.cpp");
    EXPECT_EQ(node.outputs()[0], "libtest.a");
}

TEST(BuildNodeTest, GetAttributeReturnsCorrectValue) {
    BuildNode node(
        "//src:test",
        "cc_library",
        {},
        {},
        {{"visibility", "public"}, {"optimization", "O2"}}
    );

    auto visibility = node.get_attribute("visibility");
    ASSERT_TRUE(visibility.has_value());
    EXPECT_EQ(*visibility, "public");

    auto missing = node.get_attribute("nonexistent");
    EXPECT_FALSE(missing.has_value());
}

TEST(BuildNodeTest, ComputeHashIsDeterministic) {
    BuildNode node1(
        "//src:test",
        "cc_library",
        {"file1.cpp"},
        {"libtest.a"},
        {{"key", "value"}}
    );

    BuildNode node2(
        "//src:test",
        "cc_library",
        {"file1.cpp"},
        {"libtest.a"},
        {{"key", "value"}}
    );

    EXPECT_EQ(node1.compute_hash(), node2.compute_hash());
}

TEST(BuildNodeTest, EqualityOperator) {
    BuildNode node1(
        "//src:test",
        "cc_library",
        {"file1.cpp"},
        {"libtest.a"},
        {}
    );

    BuildNode node2(
        "//src:test",
        "cc_library",
        {"file1.cpp"},
        {"libtest.a"},
        {}
    );

    BuildNode node3(
        "//src:different",
        "cc_library",
        {"file1.cpp"},
        {"libtest.a"},
        {}
    );

    EXPECT_EQ(node1, node2);
    EXPECT_NE(node1, node3);
}

// Test BuildEdge
TEST(BuildEdgeTest, ConstructorInitializesProperties) {
    BuildEdge edge("//src:app", "//src:lib", BuildEdge::DependencyType::Build);

    EXPECT_EQ(edge.from(), "//src:app");
    EXPECT_EQ(edge.to(), "//src:lib");
    EXPECT_EQ(edge.dependency_type(), BuildEdge::DependencyType::Build);
}

TEST(BuildEdgeTest, EqualityOperator) {
    BuildEdge edge1("//src:app", "//src:lib", BuildEdge::DependencyType::Build);
    BuildEdge edge2("//src:app", "//src:lib", BuildEdge::DependencyType::Build);
    BuildEdge edge3("//src:app", "//src:other", BuildEdge::DependencyType::Build);

    EXPECT_EQ(edge1, edge2);
    EXPECT_NE(edge1, edge3);
}

// Test BuildGraph Builder
TEST(BuildGraphTest, BuilderAddsNodesSuccessfully) {
    auto builder = BuildGraph::builder();
    
    auto result = builder.add_node(BuildNode(
        "//src:lib",
        "cc_library",
        {"lib.cpp"},
        {"liblib.a"},
        {}
    ));

    ASSERT_TRUE(result.has_value());
}

TEST(BuildGraphTest, BuilderRejectsNodeAlreadyExists) {
    auto builder = BuildGraph::builder();
    
    builder.add_node(BuildNode(
        "//src:lib",
        "cc_library",
        {"lib.cpp"},
        {"liblib.a"},
        {}
    ));

    auto result = builder.add_node(BuildNode(
        "//src:lib",
        "cc_library",
        {"other.cpp"},
        {"other.a"},
        {}
    ));

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), GraphError::NodeAlreadyExists);
}

TEST(BuildGraphTest, BuilderAddsEdgesSuccessfully) {
    auto builder = BuildGraph::builder();
    
    builder.add_node(BuildNode("//src:app", "cc_binary", {}, {}, {}));
    builder.add_node(BuildNode("//src:lib", "cc_library", {}, {}, {}));
    
    auto result = builder.add_edge(BuildEdge("//src:app", "//src:lib"));
    
    ASSERT_TRUE(result.has_value());
}

TEST(BuildGraphTest, BuilderRejectsEdgeToNonexistentNode) {
    auto builder = BuildGraph::builder();
    
    builder.add_node(BuildNode("//src:app", "cc_binary", {}, {}, {}));
    
    auto result = builder.add_edge(BuildEdge("//src:app", "//src:missing"));
    
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), GraphError::NodeNotFound);
}

TEST(BuildGraphTest, BuilderDetectsCycles) {
    auto builder = BuildGraph::builder();
    
    builder.add_node(BuildNode("//src:a", "cc_library", {}, {}, {}));
    builder.add_node(BuildNode("//src:b", "cc_library", {}, {}, {}));
    builder.add_node(BuildNode("//src:c", "cc_library", {}, {}, {}));
    
    builder.add_edge(BuildEdge("//src:a", "//src:b"));
    builder.add_edge(BuildEdge("//src:b", "//src:c"));
    
    // Try to create cycle: c -> a (when a -> b -> c already exists)
    auto result = builder.add_edge(BuildEdge("//src:c", "//src:a"));
    
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), GraphError::CycleDetected);
}

TEST(BuildGraphTest, BuilderCreatesValidGraph) {
    auto builder = BuildGraph::builder();
    
    builder.add_node(BuildNode("//src:app", "cc_binary", {}, {}, {}));
    builder.add_node(BuildNode("//src:lib", "cc_library", {}, {}, {}));
    builder.add_edge(BuildEdge("//src:app", "//src:lib"));
    
    auto graph_result = builder.build();
    
    ASSERT_TRUE(graph_result.has_value());
    auto& graph = graph_result.value();
    
    EXPECT_EQ(graph.node_count(), 2);
    EXPECT_EQ(graph.edge_count(), 1);
}

// Test BuildGraph queries
TEST(BuildGraphTest, GetNodeReturnsCorrectNode) {
    auto builder = BuildGraph::builder();
    builder.add_node(BuildNode("//src:lib", "cc_library", {"lib.cpp"}, {}, {}));
    auto graph = builder.build().value();
    
    auto* node = graph.get_node("//src:lib");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->label(), "//src:lib");
    EXPECT_EQ(node->node_type(), "cc_library");
}

TEST(BuildGraphTest, GetNodeReturnsNullptrForMissing) {
    auto builder = BuildGraph::builder();
    builder.add_node(BuildNode("//src:lib", "cc_library", {}, {}, {}));
    auto graph = builder.build().value();
    
    auto* node = graph.get_node("//src:missing");
    EXPECT_EQ(node, nullptr);
}

TEST(BuildGraphTest, GetAllNodesReturnsAllNodes) {
    auto builder = BuildGraph::builder();
    builder.add_node(BuildNode("//src:app", "cc_binary", {}, {}, {}));
    builder.add_node(BuildNode("//src:lib", "cc_library", {}, {}, {}));
    auto graph = builder.build().value();
    
    auto nodes = graph.get_all_nodes();
    EXPECT_EQ(nodes.size(), 2);
}

TEST(BuildGraphTest, GetDependenciesReturnsDirectDeps) {
    auto builder = BuildGraph::builder();
    builder.add_node(BuildNode("//src:app", "cc_binary", {}, {}, {}));
    builder.add_node(BuildNode("//src:lib1", "cc_library", {}, {}, {}));
    builder.add_node(BuildNode("//src:lib2", "cc_library", {}, {}, {}));
    builder.add_edge(BuildEdge("//src:app", "//src:lib1"));
    builder.add_edge(BuildEdge("//src:app", "//src:lib2"));
    auto graph = builder.build().value();
    
    auto deps = graph.get_dependencies("//src:app");
    ASSERT_TRUE(deps.has_value());
    EXPECT_EQ(deps->size(), 2);
}

TEST(BuildGraphTest, GetTransitiveDependenciesReturnsAllDeps) {
    auto builder = BuildGraph::builder();
    builder.add_node(BuildNode("//src:app", "cc_binary", {}, {}, {}));
    builder.add_node(BuildNode("//src:lib1", "cc_library", {}, {}, {}));
    builder.add_node(BuildNode("//src:lib2", "cc_library", {}, {}, {}));
    builder.add_edge(BuildEdge("//src:app", "//src:lib1"));
    builder.add_edge(BuildEdge("//src:lib1", "//src:lib2"));
    auto graph = builder.build().value();
    
    auto deps = graph.get_transitive_dependencies("//src:app");
    ASSERT_TRUE(deps.has_value());
    EXPECT_EQ(deps->size(), 2);
    EXPECT_TRUE(std::find(deps->begin(), deps->end(), "//src:lib1") != deps->end());
    EXPECT_TRUE(std::find(deps->begin(), deps->end(), "//src:lib2") != deps->end());
}

TEST(BuildGraphTest, GetDependentsReturnsReverseDeps) {
    auto builder = BuildGraph::builder();
    builder.add_node(BuildNode("//src:app", "cc_binary", {}, {}, {}));
    builder.add_node(BuildNode("//src:lib", "cc_library", {}, {}, {}));
    builder.add_edge(BuildEdge("//src:app", "//src:lib"));
    auto graph = builder.build().value();
    
    auto dependents = graph.get_dependents("//src:lib");
    ASSERT_TRUE(dependents.has_value());
    EXPECT_EQ(dependents->size(), 1);
    EXPECT_EQ((*dependents)[0], "//src:app");
}

TEST(BuildGraphTest, TopologicalSortProducesValidOrder) {
    auto builder = BuildGraph::builder();
    builder.add_node(BuildNode("//src:app", "cc_binary", {}, {}, {}));
    builder.add_node(BuildNode("//src:lib1", "cc_library", {}, {}, {}));
    builder.add_node(BuildNode("//src:lib2", "cc_library", {}, {}, {}));
    builder.add_edge(BuildEdge("//src:app", "//src:lib1"));
    builder.add_edge(BuildEdge("//src:lib1", "//src:lib2"));
    auto graph = builder.build().value();
    
    auto order = graph.topological_sort();
    EXPECT_EQ(order.size(), 3);
    
    // lib2 should come before lib1, lib1 before app
    auto lib2_pos = std::find(order.begin(), order.end(), "//src:lib2");
    auto lib1_pos = std::find(order.begin(), order.end(), "//src:lib1");
    auto app_pos = std::find(order.begin(), order.end(), "//src:app");
    
    EXPECT_LT(std::distance(order.begin(), lib2_pos), 
              std::distance(order.begin(), lib1_pos));
    EXPECT_LT(std::distance(order.begin(), lib1_pos), 
              std::distance(order.begin(), app_pos));
}

// Test serialization
TEST(BuildGraphTest, SerializeProducesValidJSON) {
    auto builder = BuildGraph::builder();
    builder.add_node(BuildNode("//src:lib", "cc_library", {"lib.cpp"}, {"lib.a"}, {}));
    auto graph = builder.build().value();
    
    auto json = graph.serialize();
    ASSERT_TRUE(json.has_value());
    
    // Check that JSON contains expected strings
    EXPECT_NE(json->find("//src:lib"), std::string::npos);
    EXPECT_NE(json->find("cc_library"), std::string::npos);
    EXPECT_NE(json->find("lib.cpp"), std::string::npos);
}

// Test deterministic behavior
TEST(BuildGraphTest, DeterministicBehavior) {
    // Build same graph twice
    auto build_graph = []() {
        auto builder = BuildGraph::builder();
        builder.add_node(BuildNode("//src:a", "cc_library", {"a.cpp"}, {}, {}));
        builder.add_node(BuildNode("//src:b", "cc_library", {"b.cpp"}, {}, {}));
        builder.add_edge(BuildEdge("//src:a", "//src:b"));
        return builder.build().value();
    };
    
    auto graph1 = build_graph();
    auto graph2 = build_graph();
    
    // Both should have same structure
    EXPECT_EQ(graph1.node_count(), graph2.node_count());
    EXPECT_EQ(graph1.edge_count(), graph2.edge_count());
    
    // Topological sort should be same
    auto order1 = graph1.topological_sort();
    auto order2 = graph2.topological_sort();
    EXPECT_EQ(order1, order2);
    
    // Serialization should be identical
    auto json1 = graph1.serialize().value();
    auto json2 = graph2.serialize().value();
    EXPECT_EQ(json1, json2);
}

} // namespace horcrux::core::test
