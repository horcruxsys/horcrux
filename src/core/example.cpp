// Horcrux - Build Graph Example
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "build_graph.h"
#include "build_node.h"
#include "build_edge.h"

#include <iostream>
#include <string>

using namespace horcrux::core;

/// @brief Example demonstrating build graph creation and queries
auto main() -> int {
    std::cout << "Horcrux Build Graph Example\n";
    std::cout << "============================\n\n";

    // Create a simple project structure:
    //   app (binary) -> lib1 (library) -> lib2 (library)
    //                -> lib3 (library)

    std::cout << "1. Creating build graph...\n";
    auto builder = BuildGraph::builder();

    // Add nodes
    std::cout << "   - Adding nodes\n";
    builder.add_node(BuildNode(
        "//src:app",
        "cc_binary",
        {"main.cpp", "app.cpp"},
        {"bin/app"},
        {{"compiler", "g++"}, {"optimization", "O2"}}
    ));

    builder.add_node(BuildNode(
        "//src:lib1",
        "cc_library",
        {"lib1.cpp", "lib1.h"},
        {"lib/liblib1.a"},
        {{"visibility", "public"}}
    ));

    builder.add_node(BuildNode(
        "//src:lib2",
        "cc_library",
        {"lib2.cpp", "lib2.h"},
        {"lib/liblib2.a"},
        {{"visibility", "public"}}
    ));

    builder.add_node(BuildNode(
        "//src:lib3",
        "cc_library",
        {"lib3.cpp", "lib3.h"},
        {"lib/liblib3.a"},
        {{"visibility", "private"}}
    ));

    // Add edges (dependencies)
    std::cout << "   - Adding dependencies\n";
    builder.add_edge(BuildEdge("//src:app", "//src:lib1"));
    builder.add_edge(BuildEdge("//src:app", "//src:lib3"));
    builder.add_edge(BuildEdge("//src:lib1", "//src:lib2"));

    // Build the graph
    std::cout << "   - Building immutable graph\n";
    auto graph_result = builder.build();
    
    if (!graph_result) {
        std::cerr << "Error building graph: " << to_string(graph_result.error()) << '\n';
        return 1;
    }

    auto& graph = graph_result.value();
    std::cout << "   ✓ Graph built successfully!\n\n";

    // Query the graph
    std::cout << "2. Graph Statistics:\n";
    std::cout << "   - Nodes: " << graph.node_count() << '\n';
    std::cout << "   - Edges: " << graph.edge_count() << "\n\n";

    // Get dependencies
    std::cout << "3. Direct dependencies of //src:app:\n";
    auto deps_result = graph.get_dependencies("//src:app");
    if (deps_result) {
        for (const auto& dep : *deps_result) {
            std::cout << "   - " << dep << '\n';
        }
    }
    std::cout << '\n';

    // Get transitive dependencies
    std::cout << "4. All transitive dependencies of //src:app:\n";
    auto all_deps_result = graph.get_transitive_dependencies("//src:app");
    if (all_deps_result) {
        for (const auto& dep : *all_deps_result) {
            std::cout << "   - " << dep << '\n';
        }
    }
    std::cout << '\n';

    // Get reverse dependencies
    std::cout << "5. What depends on //src:lib2?\n";
    auto dependents_result = graph.get_dependents("//src:lib2");
    if (dependents_result) {
        for (const auto& dependent : *dependents_result) {
            std::cout << "   - " << dependent << '\n';
        }
    }
    std::cout << '\n';

    // Topological sort
    std::cout << "6. Topological order (build order):\n";
    auto order = graph.topological_sort();
    for (size_t i = 0; i < order.size(); ++i) {
        std::cout << "   " << (i + 1) << ". " << order[i] << '\n';
    }
    std::cout << '\n';

    // Node details
    std::cout << "7. Details of //src:lib1:\n";
    auto* lib1 = graph.get_node("//src:lib1");
    if (lib1) {
        std::cout << "   - Type: " << lib1->node_type() << '\n';
        std::cout << "   - Inputs: ";
        for (const auto& input : lib1->inputs()) {
            std::cout << input << " ";
        }
        std::cout << "\n   - Outputs: ";
        for (const auto& output : lib1->outputs()) {
            std::cout << output << " ";
        }
        std::cout << "\n   - Hash: " << lib1->compute_hash() << '\n';
        
        auto visibility = lib1->get_attribute("visibility");
        if (visibility) {
            std::cout << "   - Visibility: " << *visibility << '\n';
        }
    }
    std::cout << '\n';

    // Demonstrate cycle detection
    std::cout << "8. Testing cycle detection:\n";
    auto cycle_builder = BuildGraph::builder();
    cycle_builder.add_node(BuildNode("//a", "lib", {}, {}, {}));
    cycle_builder.add_node(BuildNode("//b", "lib", {}, {}, {}));
    cycle_builder.add_node(BuildNode("//c", "lib", {}, {}, {}));
    
    cycle_builder.add_edge(BuildEdge("//a", "//b"));
    cycle_builder.add_edge(BuildEdge("//b", "//c"));
    
    std::cout << "   - Trying to add edge c->a (would create cycle)...\n";
    auto cycle_result = cycle_builder.add_edge(BuildEdge("//c", "//a"));
    
    if (!cycle_result) {
        std::cout << "   ✓ Cycle detected and prevented: " 
                  << to_string(cycle_result.error()) << "\n\n";
    }

    // Serialization
    std::cout << "9. Serializing graph to JSON:\n";
    auto json_result = graph.serialize();
    if (json_result) {
        std::cout << *json_result << '\n';
    }

    std::cout << "\nExample completed successfully!\n";
    return 0;
}
