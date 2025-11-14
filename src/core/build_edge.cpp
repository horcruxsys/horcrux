// Horcrux - Build Edge Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "build_edge.h"

namespace horcrux::core {

BuildEdge::BuildEdge(std::string from_label,
                     std::string to_label,
                     DependencyType dep_type)
    : from_label_(std::move(from_label)),
      to_label_(std::move(to_label)),
      dep_type_(dep_type) {
}

auto BuildEdge::operator==(const BuildEdge& other) const -> bool {
    return from_label_ == other.from_label_ &&
           to_label_ == other.to_label_ &&
           dep_type_ == other.dep_type_;
}

} // namespace horcrux::core
