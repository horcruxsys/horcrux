// Horcrux - Build Graph Definition
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "build_edge.h"
#include "build_node.h"

namespace horcrux::core {

/// @brief Error types for build graph operations
enum class GraphError {
  /// Node with the given label already exists
  NodeAlreadyExists,
  /// Node with the given label not found
  NodeNotFound,
  /// Edge would create a cycle in the graph
  CycleDetected,
  /// Invalid node or edge data
  InvalidInput,
  /// Serialization/deserialization error
  SerializationError
};

/// @brief Convert GraphError to human-readable string
[[nodiscard]] auto to_string(GraphError error) -> std::string;

/// @brief Immutable directed acyclic graph (DAG) for build dependencies
///
/// BuildGraph represents the complete dependency graph for a build.
/// It is immutable once constructed, ensuring thread-safety and deterministic
/// behavior. The graph enforces DAG properties (no cycles).
class BuildGraph {
public:
  using Label = BuildNode::Label;

  /// @brief Builder for constructing an immutable BuildGraph
  ///
  /// Use the Builder pattern to construct a BuildGraph. Once build() is called,
  /// the graph becomes immutable and thread-safe.
  class Builder {
  public:
    Builder() = default;

    /// @brief Add a node to the graph
    /// @param node The node to add (will be moved)
    /// @return Empty expected on success, or error if node already exists
    auto add_node(BuildNode node) -> std::expected<void, GraphError>;

    /// @brief Add an edge to the graph
    /// @param edge The edge to add (will be moved)
    /// @return Empty expected on success, or error if cycle would be created
    auto add_edge(BuildEdge edge) -> std::expected<void, GraphError>;

    /// @brief Build the immutable graph
    /// @return The constructed BuildGraph or error if validation fails
    [[nodiscard]] auto build() -> std::expected<BuildGraph, GraphError>;

  private:
    std::unordered_map<Label, std::unique_ptr<BuildNode>> nodes_;
    std::unordered_map<Label, std::vector<std::unique_ptr<BuildEdge>>> outgoing_edges_;
    std::unordered_map<Label, std::vector<std::unique_ptr<BuildEdge>>> incoming_edges_;

    /// @brief Check if adding an edge would create a cycle
    [[nodiscard]] auto would_create_cycle(const std::string& from,
                                          const std::string& to) const -> bool;

    /// @brief Depth-first search for cycle detection
    auto dfs_cycle_check(const Label& node, std::unordered_set<Label>& visited,
                         std::unordered_set<Label>& rec_stack) const -> bool;
  };

  // Delete copy constructor and assignment (immutable, use shared ownership if needed)
  BuildGraph(const BuildGraph&) = delete;
  BuildGraph& operator=(const BuildGraph&) = delete;

  // Allow move semantics
  BuildGraph(BuildGraph&&) noexcept = default;
  BuildGraph& operator=(BuildGraph&&) noexcept = default;

  ~BuildGraph() = default;

  /// @brief Create a new empty builder
  /// @return A new Builder instance
  [[nodiscard]] static auto builder() -> Builder {
    return Builder{};
  }

  /// @brief Get a node by label
  /// @param label The node label
  /// @return Pointer to node if found, nullptr otherwise
  [[nodiscard]] auto get_node(const Label& label) const -> const BuildNode*;

  /// @brief Get all nodes in the graph
  /// @return Vector of pointers to all nodes
  [[nodiscard]] auto get_all_nodes() const -> std::vector<const BuildNode*>;

  /// @brief Get direct dependencies of a node
  /// @param label The node label
  /// @return Vector of labels of direct dependencies
  [[nodiscard]] auto
  get_dependencies(const Label& label) const -> std::expected<std::vector<Label>, GraphError>;

  /// @brief Get all transitive dependencies of a node
  /// @param label The node label
  /// @return Vector of labels of all transitive dependencies in topological order
  [[nodiscard]] auto get_transitive_dependencies(const Label& label) const
      -> std::expected<std::vector<Label>, GraphError>;

  /// @brief Get direct dependents of a node (reverse dependencies)
  /// @param label The node label
  /// @return Vector of labels of direct dependents
  [[nodiscard]] auto
  get_dependents(const Label& label) const -> std::expected<std::vector<Label>, GraphError>;

  /// @brief Get topological order of all nodes
  /// @return Vector of labels in topological order
  [[nodiscard]] auto topological_sort() const -> std::vector<Label>;

  /// @brief Get number of nodes in the graph
  /// @return Node count
  [[nodiscard]] auto node_count() const -> size_t {
    return nodes_.size();
  }

  /// @brief Get number of edges in the graph
  /// @return Edge count
  [[nodiscard]] auto edge_count() const -> size_t;

  /// @brief Serialize the graph to JSON format
  /// @return JSON string representation or error
  [[nodiscard]] auto serialize() const -> std::expected<std::string, GraphError>;

  /// @brief Deserialize a graph from JSON format
  /// @param json JSON string representation
  /// @return BuildGraph or error
  [[nodiscard]] static auto
  deserialize(const std::string& json) -> std::expected<BuildGraph, GraphError>;

private:
  BuildGraph() = default;

  std::unordered_map<Label, std::unique_ptr<BuildNode>> nodes_;
  std::unordered_map<Label, std::vector<std::unique_ptr<BuildEdge>>> outgoing_edges_;
  std::unordered_map<Label, std::vector<std::unique_ptr<BuildEdge>>> incoming_edges_;

  /// @brief Helper for topological sort using DFS
  auto topological_sort_dfs(const Label& node, std::unordered_set<Label>& visited,
                            std::vector<Label>& result) const -> void;
};

} // namespace horcrux::core
