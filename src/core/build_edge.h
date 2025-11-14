// Horcrux - Build Edge Definition
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <string>

namespace horcrux::core {

/// @brief Represents a directed edge in the build dependency graph
/// 
/// A BuildEdge connects two nodes in the DAG, representing a dependency
/// relationship. Edges are immutable and lightweight.
class BuildEdge {
public:
    /// @brief Type of dependency relationship
    enum class DependencyType {
        /// Direct build dependency (e.g., library dependency)
        Build,
        /// Runtime data dependency (e.g., config files)
        Data,
        /// Tool dependency (e.g., code generator)
        Tool,
        /// Test-only dependency
        Test
    };

    /// @brief Constructor for BuildEdge
    /// @param from_label Source node label
    /// @param to_label Target node label
    /// @param dep_type Type of dependency
    BuildEdge(std::string from_label, 
              std::string to_label,
              DependencyType dep_type = DependencyType::Build);

    // Delete copy constructor and assignment to enforce immutability
    BuildEdge(const BuildEdge&) = delete;
    BuildEdge& operator=(const BuildEdge&) = delete;
    
    // Allow move semantics for efficiency
    BuildEdge(BuildEdge&&) noexcept = default;
    BuildEdge& operator=(BuildEdge&&) noexcept = default;

    ~BuildEdge() = default;

    /// @brief Get the source node label
    /// @return The from label
    [[nodiscard]] auto from() const -> const std::string& { return from_label_; }

    /// @brief Get the target node label
    /// @return The to label
    [[nodiscard]] auto to() const -> const std::string& { return to_label_; }

    /// @brief Get the dependency type
    /// @return The dependency type
    [[nodiscard]] auto dependency_type() const -> DependencyType { return dep_type_; }

    /// @brief Check if this edge equals another
    /// @param other The other edge to compare with
    /// @return True if edges are equal
    [[nodiscard]] auto operator==(const BuildEdge& other) const -> bool;

private:
    std::string from_label_;
    std::string to_label_;
    DependencyType dep_type_;
};

} // namespace horcrux::core
