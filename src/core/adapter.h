// Horcrux - Adapter Interface
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <tl/expected.hpp>

#include "build_graph.h"
#include "build_node.h"
#include "local_cache.h"

namespace horcrux::core {

/// @brief Structured diagnostic emitted by an adapter
struct Diagnostic {
  enum class Level { Info, Warning, Error };

  Level level;
  std::string message;
  std::optional<std::string> location; ///< Optional file:line reference

  [[nodiscard]] auto level_string() const -> std::string_view {
    switch (level) {
    case Level::Info:
      return "INFO";
    case Level::Warning:
      return "WARNING";
    case Level::Error:
      return "ERROR";
    }
    return "UNKNOWN";
  }
};

/// @brief Identity and capability metadata for an adapter
struct AdapterInfo {
  std::string name;                         ///< e.g., "cpp", "android"
  std::string version;                      ///< e.g., "1.0.0"
  std::vector<std::string> supported_kinds; ///< e.g., {"cc_library", "cc_binary"}
};

/// @brief A parsed and validated target configuration from an adapter
struct AdapterTarget {
  std::string label;                                               ///< e.g., "//pkg:name"
  std::string kind;                                                ///< e.g., "cc_library"
  std::unordered_map<std::string, std::vector<std::string>> attrs; ///< e.g., srcs, hdrs, deps
};

/// @brief A single build action planned by an adapter
struct BuildAction {
  enum class Kind { Compile, Archive, Link, Test, Custom };

  Kind kind;
  std::string description;
  std::vector<std::string> inputs;                  ///< Input files/paths
  std::vector<std::string> outputs;                 ///< Output files/paths
  std::vector<std::string> command;                 ///< Command to execute (argv)
  std::unordered_map<std::string, std::string> env; ///< Environment variables

  [[nodiscard]] auto kind_string() const -> std::string_view {
    switch (kind) {
    case Kind::Compile:
      return "compile";
    case Kind::Archive:
      return "archive";
    case Kind::Link:
      return "link";
    case Kind::Test:
      return "test";
    case Kind::Custom:
      return "custom";
    }
    return "unknown";
  }
};

/// @brief Error types for adapter operations
enum class AdapterError {
  UnsupportedKind, ///< Target kind not supported by this adapter
  InvalidConfig,   ///< Target configuration is invalid or missing required fields
  PlanningError,   ///< Failed to plan build actions
  ToolchainError,  ///< Toolchain not found or misconfigured
  CacheKeyError,   ///< Failed to compute cache key
};

/// @brief Convert AdapterError to human-readable string
[[nodiscard]] auto to_string(AdapterError error) -> std::string;

/// @brief Abstract base class for language adapters
///
/// An adapter is responsible for:
/// 1. Declaring which target kinds it supports
/// 2. Parsing and validating target configuration
/// 3. Planning the ordered build actions for a target
/// 4. Computing a deterministic cache key for inputs
/// 5. Emitting structured diagnostics
///
/// Adapters must be deterministic: same inputs → same actions → same outputs.
class Adapter {
public:
  virtual ~Adapter() = default;

  // Non-copyable (use shared_ptr or unique_ptr for ownership)
  Adapter(const Adapter&) = delete;
  Adapter& operator=(const Adapter&) = delete;

  // Moveable to allow factory functions
  Adapter(Adapter&&) = default;
  Adapter& operator=(Adapter&&) = default;

  /// @brief Get adapter identity and capability metadata
  [[nodiscard]] virtual auto info() const -> const AdapterInfo& = 0;

  /// @brief Check if this adapter supports the given target kind
  [[nodiscard]] virtual auto supports_kind(std::string_view kind) const -> bool;

  /// @brief Parse and validate target configuration from a BuildNode
  /// @param node The build graph node to parse
  /// @return Parsed AdapterTarget or error
  [[nodiscard]] virtual auto
  parse_target(const BuildNode& node) -> tl::expected<AdapterTarget, AdapterError> = 0;

  /// @brief Plan build actions for a target
  /// @param target The parsed and validated target
  /// @param graph  The build graph (for dependency information)
  /// @param output_root Root directory for build outputs
  /// @return Ordered list of build actions or error
  [[nodiscard]] virtual auto plan_actions(const AdapterTarget& target, const BuildGraph& graph,
                                          const std::string& output_root)
      -> tl::expected<std::vector<BuildAction>, AdapterError> = 0;

  /// @brief Compute a deterministic cache key for a target's inputs
  ///
  /// The key must be stable: same normalized inputs → same key.
  /// It should incorporate all inputs that affect the output:
  /// source file contents, flags, compiler version, etc.
  ///
  /// @param target The parsed target
  /// @return SHA-256 cache key or error
  [[nodiscard]] virtual auto
  compute_cache_key(const AdapterTarget& target) -> tl::expected<Hash, AdapterError> = 0;

  /// @brief Return accumulated diagnostics (cleared on each parse/plan call)
  [[nodiscard]] virtual auto diagnostics() const -> const std::vector<Diagnostic>& = 0;

protected:
  Adapter() = default;
};

} // namespace horcrux::core
