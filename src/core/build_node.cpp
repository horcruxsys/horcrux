// Horcrux - Build Node Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "build_node.h"

#include <algorithm>
#include <cstdint>
#include <sstream>

#include "local_cache.h"

namespace horcrux::core {

BuildNode::BuildNode(Label label, NodeType node_type, std::vector<std::string> inputs,
                     std::vector<std::string> outputs,
                     std::unordered_map<std::string, std::string> attributes)
    : label_(std::move(label)), node_type_(std::move(node_type)), inputs_(std::move(inputs)),
      outputs_(std::move(outputs)), attributes_(std::move(attributes)) {
}

auto BuildNode::get_attribute(std::string_view key) const -> std::optional<std::string> {
  // Performance: Use heterogeneous lookup to avoid string construction
  if (auto it = attributes_.find(std::string(key)); it != attributes_.end()) {
    return it->second;
  }
  return std::nullopt;
}

auto BuildNode::compute_hash() const -> Hash {
  // Optimized hash computation for performance
  // Use string builder pattern to avoid multiple allocations
  std::string content;
  content.reserve(1024); // Performance: Reserve space for typical content size

  content += label_;
  content += "|";
  content += node_type_;

  for (const auto& input : inputs_) {
    content += "|";
    content += input;
  }

  for (const auto& output : outputs_) {
    content += "|";
    content += output;
  }

  // Sort attributes for deterministic hash (performance: avoid allocation if empty)
  if (!attributes_.empty()) {
    std::vector<std::pair<std::string, std::string>> sorted_attrs;
    sorted_attrs.reserve(attributes_.size());
    sorted_attrs.assign(attributes_.begin(), attributes_.end());
    std::sort(sorted_attrs.begin(), sorted_attrs.end());

    for (const auto& attr : sorted_attrs) {
      content += "|";
      content += attr.first;
      content += "=";
      content += attr.second;
    }
  }

  // Use SHA-256 for content-based addressing
  auto raw_hash = compute_sha256(
      std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(content.data()), content.size()));
  return hash_to_string(raw_hash);
}

auto BuildNode::operator==(const BuildNode& other) const -> bool {
  return label_ == other.label_ && node_type_ == other.node_type_ && inputs_ == other.inputs_ &&
         outputs_ == other.outputs_ && attributes_ == other.attributes_;
}

} // namespace horcrux::core
