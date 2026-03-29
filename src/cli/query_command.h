// Horcrux - Query Command
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <string>
#include <string_view>

#include <tl/expected.hpp>

#include "logger.h"

namespace horcrux::cli {

/// @brief Type of query to perform
enum class QueryKind {
  Deps,         ///< Direct dependencies of a target
  TransDeps,    ///< Transitive dependencies (closure)
  Rdeps,        ///< Direct reverse dependencies (dependents)
  TopoOrder,    ///< Topological order of the full dependency closure
  AllTargets,   ///< List all targets in the graph
};

/// @brief Output format for query results
enum class QueryFormat {
  Text, ///< Human-readable text (one entry per line)
  Json, ///< Machine-readable JSON array
};

/// @brief Options for the query command
struct QueryOptions {
  std::string target;
  QueryKind kind = QueryKind::Deps;
  QueryFormat format = QueryFormat::Text;
  bool verbose = false;
};

/// @brief Error types for the query command
enum class QueryError {
  NoTargetSpecified,
  InvalidTarget,
  TargetNotFound,
  GraphError,
};

/// @brief Convert QueryError to human-readable string
[[nodiscard]] auto to_string(QueryError error) -> std::string;

/// @brief Handle the `horcrux query` command
/// @param argc Argument count
/// @param argv Argument vector
/// @param logger Logger instance
/// @return Exit code (0 = success)
auto handle_query_command(int argc, char* argv[], Logger& logger) -> int;

} // namespace horcrux::cli
