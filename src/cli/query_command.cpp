// Horcrux - Query Command Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "query_command.h"

#include <iostream>
#include <string_view>

#include "../core/build_graph.h"
#include "../core/build_node.h"
#include "build_executor.h"
#include "target_parser.h"

namespace horcrux::cli {

auto to_string(QueryError error) -> std::string {
  switch (error) {
  case QueryError::NoTargetSpecified:
    return "No target specified";
  case QueryError::InvalidTarget:
    return "Invalid target specification";
  case QueryError::TargetNotFound:
    return "Target not found in build graph";
  case QueryError::GraphError:
    return "Build graph query failed";
  }
  return "Unknown query error";
}

namespace {

void print_query_usage() {
  std::cout << "Usage: horcrux query [--deps|--trans-deps|--rdeps|--topo|--all] <target> [options]\n\n";
  std::cout << "Query build graph metadata for the specified target.\n\n";
  std::cout << "Query types:\n";
  std::cout << "  --deps         Show direct dependencies of <target> (default)\n";
  std::cout << "  --trans-deps   Show all transitive dependencies of <target>\n";
  std::cout << "  --rdeps        Show reverse dependencies (dependents) of <target>\n";
  std::cout << "  --topo         Show topological order of the dependency closure\n";
  std::cout << "  --all          List all targets in the graph\n\n";
  std::cout << "Output options:\n";
  std::cout << "  --output=text  Human-readable output (default)\n";
  std::cout << "  --output=json  Machine-readable JSON output\n\n";
  std::cout << "Other options:\n";
  std::cout << "  --verbose, -v  Enable verbose logging\n\n";
  std::cout << "Examples:\n";
  std::cout << "  horcrux query //examples/hello:app\n";
  std::cout << "  horcrux query --trans-deps //examples/hello:app\n";
  std::cout << "  horcrux query --rdeps //examples/hello:lib\n";
  std::cout << "  horcrux query --topo //examples/hello:app\n";
  std::cout << "  horcrux query --all --output=json\n";
}

/// Format a list of labels as human-readable text (one per line)
void output_text(const std::vector<std::string>& labels) {
  for (const auto& label : labels) {
    std::cout << label << "\n";
  }
}

/// Format a list of labels as a JSON array
void output_json(const std::vector<std::string>& labels) {
  std::cout << "[\n";
  for (size_t i = 0; i < labels.size(); ++i) {
    std::cout << "  \"" << labels[i] << "\"";
    if (i + 1 < labels.size()) {
      std::cout << ",";
    }
    std::cout << "\n";
  }
  std::cout << "]\n";
}

/// Output query results in the requested format
void output_results(const std::vector<std::string>& results, QueryFormat format) {
  if (format == QueryFormat::Json) {
    output_json(results);
  } else {
    output_text(results);
  }
}

} // anonymous namespace

auto handle_query_command(int argc, char* argv[], Logger& logger) -> int {
  if (argc < 2) {
    print_query_usage();
    return 1;
  }

  QueryOptions opts;

  for (int i = 2; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--deps") {
      opts.kind = QueryKind::Deps;
    } else if (arg == "--trans-deps") {
      opts.kind = QueryKind::TransDeps;
    } else if (arg == "--rdeps") {
      opts.kind = QueryKind::Rdeps;
    } else if (arg == "--topo") {
      opts.kind = QueryKind::TopoOrder;
    } else if (arg == "--all") {
      opts.kind = QueryKind::AllTargets;
    } else if (arg == "--output=text") {
      opts.format = QueryFormat::Text;
    } else if (arg == "--output=json") {
      opts.format = QueryFormat::Json;
    } else if (arg == "--verbose" || arg == "-v") {
      opts.verbose = true;
    } else if (arg == "--help" || arg == "-h") {
      print_query_usage();
      return 0;
    } else if (!arg.starts_with("--") && opts.target.empty()) {
      opts.target = std::string(arg);
    } else if (!arg.starts_with("--")) {
      logger.warning("Ignoring extra argument: ", arg);
    } else {
      logger.warning("Unknown option: ", arg);
    }
  }

  if (opts.verbose) {
    logger.set_level(LogLevel::Debug);
  }

  // AllTargets query does not require a target label
  if (opts.kind != QueryKind::AllTargets && opts.target.empty()) {
    logger.error("No target specified");
    print_query_usage();
    return 1;
  }

  // Build a demo graph (same graph used by the build command)
  auto graph_builder = core::BuildGraph::builder();
  graph_builder.add_node(
      core::BuildNode("//examples/hello:app", "cc_binary", {"examples/hello/main.cpp"},
                      {"examples/hello/app"}, {}));
  graph_builder.add_node(
      core::BuildNode("//examples/hello:lib", "cc_library",
                      {"examples/hello/lib.cpp", "examples/hello/lib.h"},
                      {"examples/hello/libhello.a"}, {}));
  graph_builder.add_node(
      core::BuildNode("//examples/simple:app", "cc_binary", {"examples/simple/main.cpp"},
                      {"examples/simple/app"}, {}));

  graph_builder.add_edge(core::BuildEdge("//examples/hello:app", "//examples/hello:lib"));

  auto graph_result = graph_builder.build();
  if (!graph_result) {
    logger.error("Failed to build graph: ",
                 core::to_string(graph_result.error()));
    return 1;
  }

  const auto& graph = *graph_result;

  // Validate target exists (unless AllTargets)
  if (opts.kind != QueryKind::AllTargets) {
    auto target_result = parse_target(opts.target);
    if (!target_result) {
      logger.error("Invalid target: ", opts.target);
      return 1;
    }

    const auto* node = graph.get_node(target_result->label());
    if (!node) {
      logger.error("Target not found in graph: ", target_result->label());
      return 1;
    }
  }

  // Execute query
  std::vector<std::string> results;

  switch (opts.kind) {
  case QueryKind::AllTargets: {
    auto nodes = graph.get_all_nodes();
    for (const auto* node : nodes) {
      results.push_back(node->label());
    }
    break;
  }

  case QueryKind::Deps: {
    auto target_result = parse_target(opts.target);
    auto deps = graph.get_dependencies(target_result->label());
    if (!deps) {
      logger.error("Graph query failed: ", core::to_string(deps.error()));
      return 1;
    }
    results = *deps;
    break;
  }

  case QueryKind::TransDeps: {
    auto target_result = parse_target(opts.target);
    auto deps = graph.get_transitive_dependencies(target_result->label());
    if (!deps) {
      logger.error("Graph query failed: ", core::to_string(deps.error()));
      return 1;
    }
    results = *deps;
    break;
  }

  case QueryKind::Rdeps: {
    auto target_result = parse_target(opts.target);
    auto rdeps = graph.get_dependents(target_result->label());
    if (!rdeps) {
      logger.error("Graph query failed: ", core::to_string(rdeps.error()));
      return 1;
    }
    results = *rdeps;
    break;
  }

  case QueryKind::TopoOrder: {
    results = graph.topological_sort();
    break;
  }
  }

  output_results(results, opts.format);
  return 0;
}

} // namespace horcrux::cli
