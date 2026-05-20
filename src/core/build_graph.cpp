// Horcrux - Build Graph Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "build_graph.h"

#include <algorithm>
#include <queue>
#include <sstream>

#include <nlohmann/json.hpp>

#include "build_edge.h"

namespace horcrux::core {

auto to_string(GraphError error) -> std::string {
  switch (error) {
  case GraphError::NodeAlreadyExists:
    return "Node with given label already exists";
  case GraphError::NodeNotFound:
    return "Node with given label not found";
  case GraphError::CycleDetected:
    return "Operation would create a cycle in the graph";
  case GraphError::InvalidInput:
    return "Invalid input data";
  case GraphError::SerializationError:
    return "Serialization or deserialization error";
  }
  return "Unknown error";
}

// Builder implementation

auto BuildGraph::Builder::add_node(BuildNode node) -> tl::expected<void, GraphError> {
  const auto label = node.label(); // Copy the label before moving

  if (nodes_.contains(label)) {
    return tl::unexpected(GraphError::NodeAlreadyExists);
  }

  nodes_[label] = std::make_unique<BuildNode>(std::move(node));
  return {};
}

auto BuildGraph::Builder::add_edge(BuildEdge edge) -> tl::expected<void, GraphError> {
  const auto from = edge.from(); // Copy before moving
  const auto to = edge.to();     // Copy before moving

  // Check if nodes exist
  if (!nodes_.contains(from) || !nodes_.contains(to)) {
    return tl::unexpected(GraphError::NodeNotFound);
  }

  // Check for cycle before adding edge
  if (would_create_cycle(from, to)) {
    return tl::unexpected(GraphError::CycleDetected);
  }

  // Add edge to both outgoing and incoming maps
  outgoing_edges_[from].push_back(std::make_unique<BuildEdge>(std::move(edge)));

  // Store a reference in incoming edges (we need to create a new edge for ownership)
  incoming_edges_[to].push_back(
      std::make_unique<BuildEdge>(from, to, outgoing_edges_[from].back()->dependency_type()));

  return {};
}

auto BuildGraph::Builder::would_create_cycle(const std::string& from,
                                             const std::string& to) const -> bool {
  // Check if adding edge from->to would create a cycle
  // This happens if there's already a path from 'to' to 'from'
  std::unordered_set<Label> visited;
  std::unordered_set<Label> rec_stack;

  // Temporarily add the edge and check for cycles starting from 'from'
  // We check if 'to' can reach 'from' in the current graph
  std::queue<Label> queue;
  queue.push(to);
  visited.insert(to);

  while (!queue.empty()) {
    Label current = queue.front();
    queue.pop();

    if (current == from) {
      return true; // Found a path from 'to' to 'from', would create cycle
    }

    auto it = outgoing_edges_.find(current);
    if (it != outgoing_edges_.end()) {
      for (const auto& edge : it->second) {
        const auto& next = edge->to();
        if (!visited.contains(next)) {
          visited.insert(next);
          queue.push(next);
        }
      }
    }
  }

  return false;
}

auto BuildGraph::Builder::dfs_cycle_check(const Label& node, std::unordered_set<Label>& visited,
                                          std::unordered_set<Label>& rec_stack) const -> bool {
  visited.insert(node);
  rec_stack.insert(node);

  auto it = outgoing_edges_.find(node);
  if (it != outgoing_edges_.end()) {
    for (const auto& edge : it->second) {
      const auto& neighbor = edge->to();

      if (!visited.contains(neighbor)) {
        if (dfs_cycle_check(neighbor, visited, rec_stack)) {
          return true;
        }
      } else if (rec_stack.contains(neighbor)) {
        return true; // Cycle detected
      }
    }
  }

  rec_stack.erase(node);
  return false;
}

auto BuildGraph::Builder::build() -> tl::expected<BuildGraph, GraphError> {
  // Validate: check for cycles in the entire graph
  std::unordered_set<Label> visited;
  std::unordered_set<Label> rec_stack;

  for (const auto& it : nodes_) {
    const auto& label = it.first;
    if (!visited.contains(label)) {
      if (dfs_cycle_check(label, visited, rec_stack)) {
        return tl::unexpected(GraphError::CycleDetected);
      }
    }
  }

  // Move data into new BuildGraph
  BuildGraph graph;
  graph.nodes_ = std::move(nodes_);
  graph.outgoing_edges_ = std::move(outgoing_edges_);
  graph.incoming_edges_ = std::move(incoming_edges_);

  return graph;
}

// BuildGraph implementation

auto BuildGraph::get_node(std::string_view label) const -> const BuildNode* {
  // Performance: Use heterogeneous lookup to avoid string construction
  if (auto it = nodes_.find(std::string(label)); it != nodes_.end()) {
    return it->second.get();
  }
  return nullptr;
}

auto BuildGraph::get_all_nodes() const -> std::vector<const BuildNode*> {
  std::vector<const BuildNode*> result;
  result.reserve(nodes_.size());

  // Performance: Direct iteration with explicit iterator
  for (const auto& it : nodes_) {
    result.push_back(it.second.get());
  }

  // Deterministic behavior: Sort by label for consistent ordering
  std::sort(result.begin(), result.end(),
            [](const BuildNode* a, const BuildNode* b) { return a->label() < b->label(); });

  return result;
}

auto BuildGraph::get_dependencies(std::string_view label) const
    -> tl::expected<std::vector<Label>, GraphError> {
  std::string label_str(label);
  if (!nodes_.contains(label_str)) {
    return tl::unexpected(GraphError::NodeNotFound);
  }

  std::vector<Label> deps;
  if (auto it = outgoing_edges_.find(label_str); it != outgoing_edges_.end()) {
    deps.reserve(it->second.size()); // Performance: Reserve capacity
    for (const auto& edge : it->second) {
      deps.push_back(edge->to());
    }
    // Deterministic behavior: Sort dependencies
    std::sort(deps.begin(), deps.end());
  }

  return deps;
}

auto BuildGraph::get_transitive_dependencies(std::string_view label) const
    -> tl::expected<std::vector<Label>, GraphError> {
  std::string label_str(label);
  if (!nodes_.contains(label_str)) {
    return tl::unexpected(GraphError::NodeNotFound);
  }

  std::vector<Label> result;
  std::unordered_set<Label> visited;
  std::queue<Label> queue;

  // Performance: Reserve capacity for common cases
  result.reserve(nodes_.size() / 4);
  visited.reserve(nodes_.size() / 4);

  queue.push(label_str);
  visited.insert(label_str);

  while (!queue.empty()) {
    Label current = queue.front();
    queue.pop();

    auto it = outgoing_edges_.find(current);
    if (it != outgoing_edges_.end()) {
      for (const auto& edge : it->second) {
        const auto& dep = edge->to();
        if (!visited.contains(dep)) {
          visited.insert(dep);
          queue.push(dep);
          result.push_back(dep);
        }
      }
    }
  }

  return result;
}

auto BuildGraph::get_dependents(std::string_view label) const
    -> tl::expected<std::vector<Label>, GraphError> {
  std::string label_str(label);
  if (!nodes_.contains(label_str)) {
    return tl::unexpected(GraphError::NodeNotFound);
  }

  std::vector<Label> dependents;
  if (auto it = incoming_edges_.find(label_str); it != incoming_edges_.end()) {
    dependents.reserve(it->second.size()); // Performance: Reserve capacity
    for (const auto& edge : it->second) {
      dependents.push_back(edge->from());
    }
    // Deterministic behavior: Sort dependents
    std::sort(dependents.begin(), dependents.end());
  }

  return dependents;
}

auto BuildGraph::topological_sort() const -> std::vector<Label> {
  std::vector<Label> result;
  std::unordered_set<Label> visited;

  for (const auto& it : nodes_) {
    const auto& label = it.first;
    if (!visited.contains(label)) {
      topological_sort_dfs(label, visited, result);
    }
  }

  // Result is already in correct topological order (dependencies first)
  return result;
}

auto BuildGraph::topological_sort_dfs(const Label& node, std::unordered_set<Label>& visited,
                                      std::vector<Label>& result) const -> void {
  visited.insert(node);

  auto it = outgoing_edges_.find(node);
  if (it != outgoing_edges_.end()) {
    for (const auto& edge : it->second) {
      const auto& neighbor = edge->to();
      if (!visited.contains(neighbor)) {
        topological_sort_dfs(neighbor, visited, result);
      }
    }
  }

  result.push_back(node);
}

auto BuildGraph::edge_count() const -> size_t {
  size_t count = 0;
  for (const auto& it : outgoing_edges_) {
    const auto& edges = it.second;
    count += edges.size();
  }
  return count;
}

auto BuildGraph::serialize() const -> tl::expected<std::string, GraphError> {
  nlohmann::json j;

  // Serialize nodes
  auto& nodes_arr = j["nodes"] = nlohmann::json::array();
  for (const auto& it : nodes_) {
    const auto& node = it.second;
    nlohmann::json node_obj;
    node_obj["label"] = node->label();
    node_obj["type"] = node->node_type();
    node_obj["inputs"] = node->inputs();
    node_obj["outputs"] = node->outputs();
    node_obj["attributes"] = node->attributes();
    nodes_arr.push_back(std::move(node_obj));
  }

  // Serialize edges
  auto& edges_arr = j["edges"] = nlohmann::json::array();
  for (const auto& it : outgoing_edges_) {
    for (const auto& edge : it.second) {
      nlohmann::json edge_obj;
      edge_obj["from"] = edge->from();
      edge_obj["to"] = edge->to();
      edge_obj["type"] = static_cast<int>(edge->dependency_type());
      edges_arr.push_back(std::move(edge_obj));
    }
  }

  return j.dump(2);
}

auto BuildGraph::deserialize(const std::string& json_str) -> tl::expected<BuildGraph, GraphError> {
  nlohmann::json j;
  try {
    j = nlohmann::json::parse(json_str);
  } catch (const nlohmann::json::parse_error&) {
    return tl::unexpected(GraphError::SerializationError);
  }

  auto builder = Builder{};

  // Deserialize nodes
  auto nodes_it = j.find("nodes");
  if (nodes_it == j.end() || !nodes_it->is_array()) {
    return tl::unexpected(GraphError::SerializationError);
  }

  for (const auto& node_obj : *nodes_it) {
    auto label = node_obj.value("label", std::string{});
    if (label.empty()) {
      return tl::unexpected(GraphError::SerializationError);
    }

    auto node_type = node_obj.value("type", std::string{});

    std::vector<std::string> inputs;
    if (auto in = node_obj.find("inputs"); in != node_obj.end() && in->is_array()) {
      inputs = in->get<std::vector<std::string>>();
    }

    std::vector<std::string> outputs;
    if (auto out = node_obj.find("outputs"); out != node_obj.end() && out->is_array()) {
      outputs = out->get<std::vector<std::string>>();
    }

    std::unordered_map<std::string, std::string> attributes;
    if (auto attrs = node_obj.find("attributes"); attrs != node_obj.end() && attrs->is_object()) {
      for (auto it = attrs->begin(); it != attrs->end(); ++it) {
        attributes[it.key()] = it->get<std::string>();
      }
    }

    auto result =
        builder.add_node(BuildNode(std::move(label), std::move(node_type), std::move(inputs),
                                   std::move(outputs), std::move(attributes)));
    if (!result) {
      return tl::unexpected(result.error());
    }
  }

  // Deserialize edges
  auto edges_it = j.find("edges");
  if (edges_it != j.end() && edges_it->is_array()) {
    for (const auto& edge_obj : *edges_it) {
      auto from = edge_obj.value("from", std::string{});
      auto to = edge_obj.value("to", std::string{});
      auto type = static_cast<BuildEdge::DependencyType>(edge_obj.value("type", 0));

      auto result = builder.add_edge(BuildEdge(std::move(from), std::move(to), type));
      if (!result) {
        return tl::unexpected(result.error());
      }
    }
  }

  return builder.build();
}

} // namespace horcrux::core
