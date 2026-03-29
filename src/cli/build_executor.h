// Horcrux - Build Executor
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "../core/build_graph.h"
#include "../core/local_cache.h"
#include "logger.h"
#include "target_parser.h"

namespace horcrux::cli {

/// @brief Error types for build execution
enum class BuildError { TargetNotFound, GraphError, CacheError, ExecutionError, InvalidTarget };

/// @brief Convert BuildError to human-readable string
[[nodiscard]] auto to_string(BuildError error) -> std::string;

/// @brief Executes builds using the core engine
class BuildExecutor {
public:
  /// @brief Create a BuildExecutor with a cache directory
  /// @param cache_dir Directory for build cache
  /// @param logger Logger instance
  static auto create(const std::filesystem::path& cache_dir,
                     Logger& logger) -> tl::expected<BuildExecutor, BuildError>;

  /// @brief Build a target
  /// @param target_spec Target specification (e.g., "//examples/hello:app")
  /// @return Empty expected on success, or error
  auto build(std::string_view target_spec) -> tl::expected<void, BuildError>;

  /// @brief Create a workspace build graph containing all known example targets
  ///
  /// Includes C++ examples (cc_library, cc_binary) and Android examples
  /// (android_binary, cc_library for JNI). In a real implementation this
  /// would be populated by parsing BUILD files in the workspace.
  static auto create_workspace_graph() -> tl::expected<core::BuildGraph, BuildError>;

private:
  BuildExecutor(core::LocalCache cache, Logger& logger)
      : cache_(std::move(cache)), logger_(logger) {
  }

  /// @brief Internal alias for create_workspace_graph() (kept for backward compat)
  auto create_demo_graph() -> tl::expected<core::BuildGraph, BuildError>;

  /// @brief Execute build for a target
  auto execute_build(const Target& target,
                     const core::BuildGraph& graph) -> tl::expected<void, BuildError>;

  /// @brief Check if target is in cache
  auto check_cache(const Target& target) -> bool;

  core::LocalCache cache_;
  Logger& logger_;
};

} // namespace horcrux::cli
