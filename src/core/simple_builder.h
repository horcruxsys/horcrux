// Horcrux - Simple Builder for Bootstrap
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

/// @brief Represents a parsed BUILD rule target
struct ParsedTarget {
  std::string rule_type;         ///< e.g. "cc_binary", "java_binary", "py_binary", "rust_binary"
  std::string name;              ///< BUILD target name
  std::vector<std::string> srcs; ///< Source file paths (relative to package)
  std::vector<std::string> deps; ///< Dependency target labels (e.g. ":greeter")
  std::string main_class;        ///< Java: main_class attribute
  std::string main_file;         ///< Python: main attribute
  std::string edition;           ///< Rust: edition attribute
};

/// @brief Simple builder with real multi-language support
class SimpleBuilder {
public:
  SimpleBuilder();

  /// @brief Build a target
  /// @param target Target label (e.g., "//examples/hello:hello")
  /// @return Success or error
  [[nodiscard]] auto build(std::string_view target) -> tl::expected<void, BuildError>;

  /// @brief Clean build artifacts
  /// @return Success or error
  [[nodiscard]] auto clean() -> tl::expected<void, BuildError>;

private:
  struct TargetInfo {
    std::string package_path;
    std::string target_name;
  };

  [[nodiscard]] auto parse_target(std::string_view target) -> tl::expected<TargetInfo, BuildError>;

  [[nodiscard]] auto build_file_exists(std::string_view package_path) -> bool;

  /// @brief Parse all named targets from a BUILD file
  [[nodiscard]] auto
  parse_all_targets(const std::filesystem::path& build_file) -> std::vector<ParsedTarget>;

  /// @brief Collect all source files for a target, resolving local package deps recursively
  [[nodiscard]] auto collect_all_sources(
      const ParsedTarget& target, const std::vector<ParsedTarget>& all_targets,
      const std::filesystem::path& package_dir) -> std::vector<std::filesystem::path>;

  /// @brief Build a cc_binary target
  [[nodiscard]] auto compile_cc_binary(const TargetInfo& info, const ParsedTarget& target)
      -> tl::expected<void, BuildError>;

  /// @brief Build a java_binary target (compiles all sources + creates launcher script)
  [[nodiscard]] auto
  build_java_binary(const TargetInfo& info, const ParsedTarget& target,
                    const std::vector<ParsedTarget>& all_targets) -> tl::expected<void, BuildError>;

  /// @brief Build a py_binary target (creates a launcher script with PYTHONPATH set)
  [[nodiscard]] auto build_py_binary(const TargetInfo& info,
                                     const ParsedTarget& target) -> tl::expected<void, BuildError>;

  /// @brief Build a rust_binary target (compiles with rustc)
  [[nodiscard]] auto build_rust_binary(const TargetInfo& info, const ParsedTarget& target)
      -> tl::expected<void, BuildError>;

  [[nodiscard]] auto get_compiler() -> const std::string&;

  [[nodiscard]] auto source_changed(const std::filesystem::path& source_file,
                                    const std::filesystem::path& output_binary) -> bool;

  std::optional<std::string> cached_compiler_;
  std::optional<LocalCache> build_cache_;
};

} // namespace horcrux::core
