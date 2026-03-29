// Horcrux - Adapter Registry
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "adapter.h"

namespace horcrux::core {

/// @brief Registry for discovering and accessing language adapters at runtime
///
/// The AdapterRegistry owns all registered adapters and provides
/// lookup by adapter name or target kind.
///
/// Thread-safety: Registration must occur before concurrent access.
class AdapterRegistry {
public:
  AdapterRegistry() = default;

  // Non-copyable; use move or shared ownership
  AdapterRegistry(const AdapterRegistry&) = delete;
  AdapterRegistry& operator=(const AdapterRegistry&) = delete;
  AdapterRegistry(AdapterRegistry&&) = default;
  AdapterRegistry& operator=(AdapterRegistry&&) = default;

  /// @brief Register an adapter
  /// @param adapter Owning pointer to the adapter
  void register_adapter(std::unique_ptr<Adapter> adapter);

  /// @brief Find an adapter by name
  /// @param name Adapter name (e.g., "cpp")
  /// @return Pointer to adapter, or nullptr if not found
  [[nodiscard]] auto find_by_name(std::string_view name) const -> const Adapter*;

  /// @brief Find an adapter that supports the given target kind
  /// @param kind Target kind (e.g., "cc_library")
  /// @return Pointer to adapter, or nullptr if not supported
  [[nodiscard]] auto find_for_kind(std::string_view kind) const -> const Adapter*;

  /// @brief Find a mutable adapter that supports the given target kind
  [[nodiscard]] auto find_for_kind_mut(std::string_view kind) -> Adapter*;

  /// @brief Get all registered adapters
  [[nodiscard]] auto all_adapters() const -> std::vector<const Adapter*>;

  /// @brief Get the total number of registered adapters
  [[nodiscard]] auto size() const -> size_t {
    return adapters_.size();
  }

private:
  std::vector<std::unique_ptr<Adapter>> adapters_;
};

} // namespace horcrux::core
