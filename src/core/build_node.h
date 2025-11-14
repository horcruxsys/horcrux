// Horcrux - Build Node Definition
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace horcrux::core {

// Forward declaration
class BuildEdge;

/// @brief Represents a node in the build dependency graph
///
/// A BuildNode is an immutable representation of a build target with its
/// properties and metadata. Following the immutability principle from our
/// architecture for thread-safety and correctness.
class BuildNode {
public:
  /// @brief Unique identifier for the build node (e.g., "//src/app:main")
  using Label = std::string;

  /// @brief Hash value for content-based addressing
  using Hash = std::string;

  /// @brief Type of build node (e.g., "cc_binary", "cc_library", "py_test")
  using NodeType = std::string;

  /// @brief Constructor for BuildNode
  /// @param label Unique label identifying this node
  /// @param node_type Type of build node
  /// @param inputs Input files/resources for this node
  /// @param outputs Expected output files from building this node
  /// @param attributes Additional metadata for the node
  BuildNode(Label label, NodeType node_type, std::vector<std::string> inputs,
            std::vector<std::string> outputs,
            std::unordered_map<std::string, std::string> attributes = {});

  // Delete copy constructor and assignment to enforce immutability
  BuildNode(const BuildNode&) = delete;
  BuildNode& operator=(const BuildNode&) = delete;

  // Allow move semantics for efficiency
  BuildNode(BuildNode&&) noexcept = default;
  BuildNode& operator=(BuildNode&&) noexcept = default;

  ~BuildNode() = default;

  /// @brief Get the unique label of this node
  /// @return The label string
  [[nodiscard]] auto label() const -> const Label& {
    return label_;
  }

  /// @brief Get the type of this node
  /// @return The node type string
  [[nodiscard]] auto node_type() const -> const NodeType& {
    return node_type_;
  }

  /// @brief Get the input files/resources
  /// @return Vector of input paths
  [[nodiscard]] auto inputs() const -> const std::vector<std::string>& {
    return inputs_;
  }

  /// @brief Get the expected output files
  /// @return Vector of output paths
  [[nodiscard]] auto outputs() const -> const std::vector<std::string>& {
    return outputs_;
  }

  /// @brief Get all attributes
  /// @return Map of attribute key-value pairs
  [[nodiscard]] auto attributes() const -> const std::unordered_map<std::string, std::string>& {
    return attributes_;
  }

  /// @brief Get a specific attribute value
  /// @param key The attribute key
  /// @return Optional containing the value if found
  [[nodiscard]] auto get_attribute(const std::string& key) const -> std::optional<std::string>;

  /// @brief Compute content hash for this node
  /// @return Hash string for content-based addressing
  [[nodiscard]] auto compute_hash() const -> Hash;

  /// @brief Check if this node equals another
  /// @param other The other node to compare with
  /// @return True if nodes are equal
  [[nodiscard]] auto operator==(const BuildNode& other) const -> bool;

private:
  Label label_;
  NodeType node_type_;
  std::vector<std::string> inputs_;
  std::vector<std::string> outputs_;
  std::unordered_map<std::string, std::string> attributes_;
};

} // namespace horcrux::core
