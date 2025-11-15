// Horcrux - Simple Builder for Bootstrap
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <string>
#include <string_view>

#include <tl/expected.hpp>

namespace horcrux::core {

enum class BuildError {
  InvalidTarget,
  CompilationFailed,
  SourceNotFound,
  BuildFileNotFound,
};

/// @brief Convert BuildError to human-readable string
[[nodiscard]] auto to_string(BuildError error) -> std::string;

/// @brief Simple builder for bootstrapping
/// This is a minimal implementation to demonstrate end-to-end builds
/// A full implementation will use the BuildGraph and caching infrastructure
class SimpleBuilder {
public:
  SimpleBuilder() = default;

  /// @brief Build a target (simplified implementation)
  /// @param target Target label (e.g., "//examples/hello:hello")
  /// @return Success or error
  [[nodiscard]] auto build(std::string_view target) -> tl::expected<void, BuildError>;

  /// @brief Clean build artifacts
  /// @return Success or error
  [[nodiscard]] auto clean() -> tl::expected<void, BuildError>;

private:
  /// @brief Parse target label into package and name
  struct TargetInfo {
    std::string package_path;
    std::string target_name;
  };

  [[nodiscard]] auto parse_target(std::string_view target) -> tl::expected<TargetInfo, BuildError>;

  /// @brief Check if BUILD file exists
  [[nodiscard]] auto build_file_exists(std::string_view package_path) -> bool;

  /// @brief Simple compile for cc_binary targets
  [[nodiscard]] auto
  compile_cc_binary(const TargetInfo& target_info) -> tl::expected<void, BuildError>;
};

} // namespace horcrux::core
