// Horcrux - Target Parser
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <tl/expected.hpp>

namespace horcrux::cli {

/// @brief Error types for target parsing
enum class TargetParseError { InvalidFormat, EmptyTarget, MissingPackage, MissingTargetName };

/// @brief Convert TargetParseError to human-readable string
[[nodiscard]] auto to_string(TargetParseError error) -> std::string;

/// @brief Represents a parsed build target
struct Target {
  std::string package;     // e.g., "examples/hello"
  std::string target_name; // e.g., "app"

  /// @brief Get the full label (e.g., "//examples/hello:app")
  [[nodiscard]] auto label() const -> std::string {
    return "//" + package + ":" + target_name;
  }
};

/// @brief Parse a Bazel-style target specification
/// @param target_spec Target specification (e.g., "//examples/hello:app")
/// @return Parsed Target or error
///
/// Supported formats:
/// - //package:target  - Full specification
/// - //package         - Package with implicit target (same as package name)
/// - :target           - Target in current package (not yet supported)
[[nodiscard]] auto
parse_target(std::string_view target_spec) -> tl::expected<Target, TargetParseError>;

} // namespace horcrux::cli
