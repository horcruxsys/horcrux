// Horcrux - Adapter Interface Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "adapter.h"

namespace horcrux::core {

auto to_string(AdapterError error) -> std::string {
  switch (error) {
  case AdapterError::UnsupportedKind:
    return "Target kind not supported by this adapter";
  case AdapterError::InvalidConfig:
    return "Invalid or incomplete target configuration";
  case AdapterError::PlanningError:
    return "Failed to plan build actions";
  case AdapterError::ToolchainError:
    return "Toolchain not found or misconfigured";
  case AdapterError::CacheKeyError:
    return "Failed to compute cache key";
  }
  return "Unknown adapter error";
}

auto Adapter::supports_kind(std::string_view kind) const -> bool {
  const auto& supported = info().supported_kinds;
  for (const auto& k : supported) {
    if (k == kind) {
      return true;
    }
  }
  return false;
}

} // namespace horcrux::core
