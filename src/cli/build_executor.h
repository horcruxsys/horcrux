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
#include "../core/repro_checker.h"
#include "../core/sandbox_policy.h"
#include "logger.h"
#include "target_parser.h"

namespace horcrux::cli {

/// @brief Error types for build execution
enum class BuildError {
  TargetNotFound,
  GraphError,
  CacheError,
  ExecutionError,
  InvalidTarget,
  PolicyError,     ///< Sandbox policy parsing or validation error
  ReproCheckFailed ///< Repro-check mode detected non-reproducible outputs
};

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

  /// @brief Create a BuildExecutor with explicit sandbox policy
  /// @param cache_dir   Directory for build cache
  /// @param policy      Hermetic sandbox policy to apply
  /// @param repro_check If true, enable double-build reproducibility check
  /// @param logger      Logger instance
  static auto create_with_policy(const std::filesystem::path& cache_dir,
                                 core::SandboxPolicy policy, bool repro_check,
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

  /// @brief Return the active sandbox policy
  [[nodiscard]] auto policy() const -> const core::SandboxPolicy&;

  /// @brief Return whether repro-check mode is active
  [[nodiscard]] auto repro_check_enabled() const -> bool;

private:
  BuildExecutor(core::LocalCache cache, core::SandboxPolicy policy, bool repro_check,
                Logger& logger)
      : cache_(std::move(cache)), policy_(std::move(policy)), repro_check_(repro_check),
        logger_(logger) {}

  /// @brief Internal alias for create_workspace_graph() (kept for backward compat)
  auto create_demo_graph() -> tl::expected<core::BuildGraph, BuildError>;

  /// @brief Execute build for a target
  auto execute_build(const Target& target,
                     const core::BuildGraph& graph) -> tl::expected<void, BuildError>;

  /// @brief Check if target is in cache (policy-fingerprinted)
  auto check_cache(const Target& target) -> bool;

  core::LocalCache cache_;
  core::SandboxPolicy policy_;
  bool repro_check_ = false;
  Logger& logger_;
};

} // namespace horcrux::cli
