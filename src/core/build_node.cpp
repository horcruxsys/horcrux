// Horcrux - Build Node Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "build_node.h"

#include <algorithm>
#include <functional>
#include <sstream>

namespace horcrux::core {

BuildNode::BuildNode(Label label, NodeType node_type, std::vector<std::string> inputs,
                     std::vector<std::string> outputs,
                     std::unordered_map<std::string, std::string> attributes)
    : label_(std::move(label)), node_type_(std::move(node_type)), inputs_(std::move(inputs)),
      outputs_(std::move(outputs)), attributes_(std::move(attributes)) {
}

auto BuildNode::get_attribute(const std::string& key) const -> std::optional<std::string> {
  auto it = attributes_.find(key);
  if (it != attributes_.end()) {
    return it->second;
  }
  return std::nullopt;
}

auto BuildNode::compute_hash() const -> Hash {
  // Simple hash computation based on node properties
  // In production, this would use a proper cryptographic hash like SHA-256
  std::ostringstream oss;
  oss << label_ << "|" << node_type_;

  for (const auto& input : inputs_) {
    oss << "|" << input;
  }

  for (const auto& output : outputs_) {
    oss << "|" << output;
  }

  // Sort attributes for deterministic hash
  std::vector<std::pair<std::string, std::string>> sorted_attrs(attributes_.begin(),
                                                                attributes_.end());
  std::sort(sorted_attrs.begin(), sorted_attrs.end());

  for (const auto& [key, value] : sorted_attrs) {
    oss << "|" << key << "=" << value;
  }

  // Use std::hash for now (production would use proper crypto hash)
  std::string content = oss.str();
  std::hash<std::string> hasher;
  size_t hash_value = hasher(content);

  std::ostringstream hash_stream;
  hash_stream << std::hex << hash_value;
  return hash_stream.str();
}

auto BuildNode::operator==(const BuildNode& other) const -> bool {
  return label_ == other.label_ && node_type_ == other.node_type_ && inputs_ == other.inputs_ &&
         outputs_ == other.outputs_ && attributes_ == other.attributes_;
}

} // namespace horcrux::core
