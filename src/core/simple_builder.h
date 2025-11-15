// Horcrux - Simple Builder for Bootstrap
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <tl/expected.hpp>

#include "local_cache.h"

namespace horcrux::core {

enum class BuildError {
  InvalidTarget,
  CompilationFailed,
  SourceNotFound,
  BuildFileNotFound,
  CacheError,
};

/// @brief Convert BuildError to human-readable string
[[nodiscard]] auto to_string(BuildError error) -> std::string;

/// @brief Simple builder for bootstrapping with caching support
/// This implementation demonstrates end-to-end builds with LocalCache integration
/// for significant performance improvements
class SimpleBuilder {
public:
  SimpleBuilder();

  /// @brief Build a target (optimized with caching)
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

  /// @brief Optimized compile for cc_binary targets with caching
  [[nodiscard]] auto
  compile_cc_binary(const TargetInfo& target_info) -> tl::expected<void, BuildError>;

  /// @brief Get cached compiler path (avoids repeated system calls)
  [[nodiscard]] auto get_compiler() -> const std::string&;

  /// @brief Check if source has changed (for incremental builds)
  [[nodiscard]] auto source_changed(const std::filesystem::path& source_file,
                                     const std::filesystem::path& output_binary) -> bool;

  // Cache the compiler path to avoid repeated system() calls
  std::optional<std::string> cached_compiler_;

  // Build cache for storing compiled artifacts
  std::optional<LocalCache> build_cache_;
};

} // namespace horcrux::core
