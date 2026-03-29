// Horcrux - Adapter Registry Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "adapter_registry.h"

namespace horcrux::core {

void AdapterRegistry::register_adapter(std::unique_ptr<Adapter> adapter) {
  adapters_.push_back(std::move(adapter));
}

auto AdapterRegistry::find_by_name(std::string_view name) const -> const Adapter* {
  for (const auto& adapter : adapters_) {
    if (adapter->info().name == name) {
      return adapter.get();
    }
  }
  return nullptr;
}

auto AdapterRegistry::find_for_kind(std::string_view kind) const -> const Adapter* {
  for (const auto& adapter : adapters_) {
    if (adapter->supports_kind(kind)) {
      return adapter.get();
    }
  }
  return nullptr;
}

auto AdapterRegistry::find_for_kind_mut(std::string_view kind) -> Adapter* {
  for (auto& adapter : adapters_) {
    if (adapter->supports_kind(kind)) {
      return adapter.get();
    }
  }
  return nullptr;
}

auto AdapterRegistry::all_adapters() const -> std::vector<const Adapter*> {
  std::vector<const Adapter*> result;
  result.reserve(adapters_.size());
  for (const auto& adapter : adapters_) {
    result.push_back(adapter.get());
  }
  return result;
}

} // namespace horcrux::core
