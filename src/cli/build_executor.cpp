// Horcrux - Build Executor Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "build_executor.h"

#include <chrono>
#include <thread>

#include "../core/build_edge.h"
#include "../core/build_node.h"

namespace horcrux::cli {

auto to_string(BuildError error) -> std::string {
  switch (error) {
  case BuildError::TargetNotFound:
    return "Target not found in build graph";
  case BuildError::GraphError:
    return "Error creating or querying build graph";
  case BuildError::CacheError:
    return "Error accessing build cache";
  case BuildError::ExecutionError:
    return "Error executing build";
  case BuildError::InvalidTarget:
    return "Invalid target specification";
  }
  return "Unknown error";
}

auto BuildExecutor::create(const std::filesystem::path& cache_dir,
                           Logger& logger) -> tl::expected<BuildExecutor, BuildError> {
  auto cache_result = core::LocalCache::create(cache_dir);
  if (!cache_result) {
    logger.error("Failed to create cache: ", cache_dir.string());
    return tl::unexpected(BuildError::CacheError);
  }

  return BuildExecutor{std::move(*cache_result), logger};
}

auto BuildExecutor::create_workspace_graph() -> tl::expected<core::BuildGraph, BuildError> {
  // Create a workspace build graph representing all known example targets.
  // In a real implementation this would be populated by parsing BUILD files.
  auto builder = core::BuildGraph::builder();

  // ── C++ examples ────────────────────────────────────────────────────────
  builder.add_node(core::BuildNode("//examples/hello:app", "cc_binary", {"examples/hello/main.cpp"},
                                   {"examples/hello/app"}, {}));
  builder.add_node(core::BuildNode("//examples/hello:lib", "cc_library",
                                   {"examples/hello/lib.cpp", "examples/hello/lib.h"},
                                   {"examples/hello/libhello.a"}, {}));
  builder.add_node(core::BuildNode("//examples/simple:app", "cc_binary",
                                   {"examples/simple/main.cpp"}, {"examples/simple/app"}, {}));

  // C++ dep: hello:app → hello:lib
  builder.add_edge(core::BuildEdge("//examples/hello:app", "//examples/hello:lib"));

  // ── Android examples ────────────────────────────────────────────────────
  // basic-xml-app
  builder.add_node(core::BuildNode(
      "//examples/android/basic-xml-app:app", "android_binary",
      {"src/main/java", "src/main/AndroidManifest.xml", "src/main/res"}, {"basic-xml-app.apk"},
      {{"min_sdk_version", "21"}, {"target_sdk_version", "34"}}));

  // compose-app
  builder.add_node(core::BuildNode("//examples/android/compose-app:app", "android_binary",
                                   {"src/main/java", "src/main/AndroidManifest.xml"},
                                   {"compose-app.apk"},
                                   {{"min_sdk_version", "24"}, {"target_sdk_version", "34"}}));

  // multi-flavor-app
  builder.add_node(core::BuildNode(
      "//examples/android/multi-flavor-app:app", "android_binary",
      {"src/main/java", "src/main/AndroidManifest.xml", "src/main/res"}, {"multi-flavor-app.apk"},
      {{"min_sdk_version", "21"}, {"target_sdk_version", "34"}}));

  // ndk-app – native cc_library + android_binary
  builder.add_node(
      core::BuildNode("//examples/android/ndk-app:native_lib", "cc_library",
                      {"src/main/cpp/native-lib.cpp"}, {"libnative_lib.so"},
                      {{"copts", "-std=c++17 -Wall -Wextra -ffast-math"}, {"linkopts", "-llog"}}));
  builder.add_node(
      core::BuildNode("//examples/android/ndk-app:app", "android_binary",
                      {"src/main/java", "src/main/AndroidManifest.xml", "src/main/res"},
                      {"ndk-app.apk"}, {{"min_sdk_version", "21"}, {"target_sdk_version", "34"}}));
  // ndk-app:app depends on ndk-app:native_lib
  builder.add_edge(
      core::BuildEdge("//examples/android/ndk-app:app", "//examples/android/ndk-app:native_lib"));

  // multi-module-app (top-level app)
  builder.add_node(core::BuildNode("//examples/android/multi-module-app:app", "android_binary",
                                   {"app/src/main/java", "app/src/main/AndroidManifest.xml"},
                                   {"multi-module-app.apk"},
                                   {{"min_sdk_version", "21"}, {"target_sdk_version", "34"}}));

  // ── Rust examples ─────────────────────────────────────────────────────
  builder.add_node(core::BuildNode("//examples/rust:greet", "rust_library",
                                   {"examples/rust/src/greet.rs"}, {"examples/rust/libgreet.rlib"},
                                   {{"edition", "2021"}}));
  builder.add_node(core::BuildNode("//examples/rust:hello", "rust_binary",
                                   {"examples/rust/src/main.rs"}, {"examples/rust/hello"},
                                   {{"edition", "2021"}}));
  builder.add_node(core::BuildNode("//examples/rust:greet_test", "rust_test",
                                   {"examples/rust/src/greet_test.rs"}, {}, {{"edition", "2021"}}));

  // Rust deps
  builder.add_edge(core::BuildEdge("//examples/rust:hello", "//examples/rust:greet"));
  builder.add_edge(core::BuildEdge("//examples/rust:greet_test", "//examples/rust:greet"));

  // ── Python examples ────────────────────────────────────────────────────
  builder.add_node(core::BuildNode("//examples/python:greet", "py_library",
                                   {"examples/python/src/greet.py"}, {}, {}));
  builder.add_node(core::BuildNode("//examples/python:hello", "py_binary",
                                   {"examples/python/src/main.py"}, {},
                                   {{"main", "examples/python/src/main.py"}}));
  builder.add_node(core::BuildNode("//examples/python:greet_test", "py_test",
                                   {"examples/python/src/greet_test.py"}, {},
                                   {{"main", "examples/python/src/greet_test.py"}}));

  // Python deps
  builder.add_edge(core::BuildEdge("//examples/python:hello", "//examples/python:greet"));
  builder.add_edge(core::BuildEdge("//examples/python:greet_test", "//examples/python:greet"));

  // ── Java examples (non-Android) ────────────────────────────────────────
  builder.add_node(core::BuildNode("//examples/java:greeter", "java_library",
                                   {"examples/java/src/com/example/Greeter.java"},
                                   {"examples/java/greeter.jar"}, {}));
  builder.add_node(core::BuildNode(
      "//examples/java:hello", "java_binary", {"examples/java/src/com/example/Main.java"},
      {"examples/java/hello.jar"}, {{"main_class", "com.example.Main"}}));
  builder.add_node(core::BuildNode("//examples/java:greeter_test", "java_test",
                                   {"examples/java/src/com/example/GreeterTest.java"}, {},
                                   {{"main_class", "com.example.GreeterTest"}}));

  // Java deps
  builder.add_edge(core::BuildEdge("//examples/java:hello", "//examples/java:greeter"));
  builder.add_edge(core::BuildEdge("//examples/java:greeter_test", "//examples/java:greeter"));

  auto graph_result = builder.build();
  if (!graph_result) {
    return tl::unexpected(BuildError::GraphError);
  }
  return std::move(*graph_result);
}

auto BuildExecutor::create_demo_graph() -> tl::expected<core::BuildGraph, BuildError> {
  return create_workspace_graph();
}

auto BuildExecutor::check_cache(const Target& target) -> bool {
  // Create a hash for the target
  // In a real implementation, this would include source file hashes
  std::string target_key = target.label();
  std::vector<uint8_t> key_bytes(target_key.begin(), target_key.end());

  auto hash = core::compute_sha256(key_bytes);
  return cache_.contains(hash);
}

auto BuildExecutor::execute_build(const Target& target,
                                  const core::BuildGraph& graph) -> tl::expected<void, BuildError> {
  auto label = target.label();

  // Check if target exists in graph
  auto* node = graph.get_node(label);
  if (!node) {
    logger_.error("Target not found: ", label);
    return tl::unexpected(BuildError::TargetNotFound);
  }

  // Check cache first
  if (check_cache(target)) {
    logger_.info("✓ Target ", label, " found in cache (instant rebuild)");
    return {};
  }

  logger_.info("Building target: ", label);
  logger_.info("  Type: ", node->node_type());

  // Get dependencies
  auto deps_result = graph.get_transitive_dependencies(label);
  if (!deps_result) {
    logger_.error("Failed to get dependencies for ", label);
    return tl::unexpected(BuildError::GraphError);
  }

  // Build dependencies first (topological order)
  for (const auto& dep_label : *deps_result) {
    auto* dep_node = graph.get_node(dep_label);
    if (dep_node) {
      logger_.info("  Building dependency: ", dep_label);

      // Simulate build time
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  // Build the target
  logger_.info("  Compiling ", node->inputs().size(), " source file(s)");

  // Simulate build
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  logger_.info("  Linking ", node->outputs().size(), " output(s)");

  // Simulate linking
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Store in cache
  std::string target_key = target.label();
  std::vector<uint8_t> key_bytes(target_key.begin(), target_key.end());
  auto hash = core::compute_sha256(key_bytes);

  // Create a dummy artifact
  std::vector<uint8_t> artifact_data{'b', 'u', 'i', 'l', 't'};
  core::Artifact artifact{artifact_data,
                          std::chrono::system_clock::now().time_since_epoch().count()};

  auto store_result = cache_.store(hash, artifact);
  if (!store_result) {
    logger_.warning("Failed to cache build result");
  }

  logger_.info("✓ Successfully built ", label);
  return {};
}

auto BuildExecutor::build(std::string_view target_spec) -> tl::expected<void, BuildError> {
  logger_.info("Parsing target: ", target_spec);

  // Parse the target
  auto target_result = parse_target(target_spec);
  if (!target_result) {
    logger_.error("Failed to parse target: ", to_string(target_result.error()));
    return tl::unexpected(BuildError::InvalidTarget);
  }

  auto target = std::move(*target_result);
  logger_.debug("Parsed target - package: ", target.package, ", name: ", target.target_name);

  // Create the build graph
  logger_.info("Loading build graph...");
  auto graph_result = create_demo_graph();
  if (!graph_result) {
    logger_.error("Failed to create build graph");
    return tl::unexpected(BuildError::GraphError);
  }

  auto& graph = *graph_result;
  logger_.debug("Graph loaded: ", graph.node_count(), " nodes, ", graph.edge_count(), " edges");

  // Execute the build
  return execute_build(target, graph);
}

} // namespace horcrux::cli
