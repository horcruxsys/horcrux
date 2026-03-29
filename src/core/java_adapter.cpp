// Horcrux - Java Adapter Implementation (non-Android)
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "java_adapter.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <sstream>

#include "local_cache.h"

namespace horcrux::core {

namespace {

constexpr std::string_view kAdapterName = "java";
constexpr std::string_view kAdapterVersion = "1.0.0";
constexpr std::string_view kDefaultSourceVersion = "11";

/// Java source file extensions
constexpr std::array<std::string_view, 1> kJavaSourceExtensions = {".java"};

/// Search PATH for an executable
auto executable_on_path(std::string_view name) -> bool {
  const char* path_env = std::getenv("PATH");
  if (path_env == nullptr) {
    return false;
  }
  std::string path_str(path_env);
  std::istringstream stream(path_str);
  std::string dir;
  while (std::getline(stream, dir, ':')) {
    if (std::filesystem::exists(std::filesystem::path(dir) / name)) {
      return true;
    }
  }
  return false;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

auto JavaAdapter::label_target_name(const std::string& label) -> std::string {
  auto colon = label.rfind(':');
  if (colon != std::string::npos) {
    return label.substr(colon + 1);
  }
  auto slash = label.rfind('/');
  if (slash != std::string::npos) {
    return label.substr(slash + 1);
  }
  return label;
}

auto JavaAdapter::label_to_safe_path(const std::string& label) -> std::string {
  std::string result = label;
  if (result.size() >= 2 && result[0] == '/' && result[1] == '/') {
    result = result.substr(2);
  }
  std::replace(result.begin(), result.end(), '/', '_');
  std::replace(result.begin(), result.end(), ':', '_');
  return result;
}

auto JavaAdapter::split_tokens(const std::string& value) -> std::vector<std::string> {
  std::vector<std::string> tokens;
  std::istringstream stream(value);
  std::string token;
  while (stream >> token) {
    if (!token.empty() && token.back() == ',') {
      token.pop_back();
    }
    if (!token.empty()) {
      tokens.push_back(std::move(token));
    }
  }
  return tokens;
}

auto JavaAdapter::filter_java_sources(const std::vector<std::string>& files)
    -> std::vector<std::string> {
  std::vector<std::string> sources;
  for (const auto& f : files) {
    auto ext = std::filesystem::path(f).extension().string();
    for (const auto& valid_ext : kJavaSourceExtensions) {
      if (ext == valid_ext) {
        sources.push_back(f);
        break;
      }
    }
  }
  return sources;
}

auto JavaAdapter::collect_classpath(const AdapterTarget& target, const BuildGraph& graph,
                                    const std::string& output_root) -> std::vector<std::string> {
  std::vector<std::string> cp;

  // Add classpath from explicit deps attribute
  const auto& deps_it = target.attrs.find("deps");
  if (deps_it != target.attrs.end()) {
    for (const auto& dep_label : deps_it->second) {
      const std::string dep_safe = label_to_safe_path(dep_label);
      const std::string dep_name = label_target_name(dep_label);
      cp.push_back(output_root + "/jar/" + dep_safe + "/" + dep_name + ".jar");
    }
  }

  // Also collect from graph direct deps
  auto graph_deps = graph.get_dependencies(target.label);
  if (graph_deps) {
    for (const auto& dep_label : *graph_deps) {
      const std::string dep_safe = label_to_safe_path(dep_label);
      const std::string dep_name = label_target_name(dep_label);
      const std::string dep_jar = output_root + "/jar/" + dep_safe + "/" + dep_name + ".jar";
      // Avoid duplicates
      if (std::find(cp.begin(), cp.end(), dep_jar) == cp.end()) {
        cp.push_back(dep_jar);
      }
    }
  }

  return cp;
}

// ─────────────────────────────────────────────────────────────────────────────
// Toolchain detection
// ─────────────────────────────────────────────────────────────────────────────

auto JavaAdapter::detect_toolchain() -> tl::expected<JavaToolchain, AdapterError> {
  JavaToolchain tc;

  const char* env_javac = std::getenv("HORCRUX_JAVAC");
  if (env_javac != nullptr && std::filesystem::exists(env_javac)) {
    tc.javac = std::string(env_javac);
  } else if (executable_on_path("javac")) {
    tc.javac = "javac";
  } else {
    return tl::unexpected(AdapterError::ToolchainError);
  }

  tc.java = executable_on_path("java") ? "java" : "";
  tc.jar_tool = executable_on_path("jar") ? "jar" : "jar";
  tc.java_version = "java-unknown";
  tc.source_version = std::string(kDefaultSourceVersion);
  tc.target_version = std::string(kDefaultSourceVersion);
  return tc;
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory and constructor
// ─────────────────────────────────────────────────────────────────────────────

auto JavaAdapter::create() -> tl::expected<JavaAdapter, AdapterError> {
  auto tc = detect_toolchain();
  if (!tc) {
    return tl::unexpected(tc.error());
  }
  return JavaAdapter{std::move(*tc)};
}

JavaAdapter::JavaAdapter(JavaToolchain toolchain) : toolchain_(std::move(toolchain)) {
  info_.name = std::string(kAdapterName);
  info_.version = std::string(kAdapterVersion);
  info_.supported_kinds = {"java_library", "java_binary", "java_test"};
}

// ─────────────────────────────────────────────────────────────────────────────
// Adapter interface
// ─────────────────────────────────────────────────────────────────────────────

auto JavaAdapter::info() const -> const AdapterInfo& {
  return info_;
}

auto JavaAdapter::diagnostics() const -> const std::vector<Diagnostic>& {
  return diagnostics_;
}

void JavaAdapter::emit(Diagnostic::Level level, std::string message,
                       std::optional<std::string> location) const {
  diagnostics_.push_back(Diagnostic{level, std::move(message), std::move(location)});
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_target
// ─────────────────────────────────────────────────────────────────────────────

auto JavaAdapter::parse_target(const BuildNode& node) -> tl::expected<AdapterTarget, AdapterError> {
  diagnostics_.clear();

  const auto& kind = node.node_type();
  if (!supports_kind(kind)) {
    emit(Diagnostic::Level::Error, "Unsupported target kind: " + kind);
    return tl::unexpected(AdapterError::UnsupportedKind);
  }

  AdapterTarget target;
  target.label = node.label();
  target.kind = kind;

  target.attrs["srcs"] = filter_java_sources(node.inputs());

  auto deps_attr = node.get_attribute("deps");
  if (deps_attr) {
    target.attrs["deps"] = split_tokens(*deps_attr);
  }

  auto javacopts_attr = node.get_attribute("javacopts");
  if (javacopts_attr) {
    target.attrs["javacopts"] = split_tokens(*javacopts_attr);
  }

  // Main class for java_binary / java_test
  auto main_class_attr = node.get_attribute("main_class");
  if (main_class_attr) {
    target.attrs["main_class"] = {*main_class_attr};
  }

  return target;
}

// ─────────────────────────────────────────────────────────────────────────────
// plan_actions
// ─────────────────────────────────────────────────────────────────────────────

auto JavaAdapter::plan_compile_action(const AdapterTarget& target, const BuildGraph& graph,
                                      const std::string& output_root) const
    -> tl::expected<BuildAction, AdapterError> {
  const auto& srcs_it = target.attrs.find("srcs");
  if (srcs_it == target.attrs.end() || srcs_it->second.empty()) {
    emit(Diagnostic::Level::Error, "java target has no .java sources: " + target.label);
    return tl::unexpected(AdapterError::PlanningError);
  }

  const std::string safe = label_to_safe_path(target.label);
  const std::string classes_dir = output_root + "/classes/" + safe;

  auto classpath = collect_classpath(target, graph, output_root);

  BuildAction action;
  action.kind = BuildAction::Kind::Compile;
  action.description = "Compiling Java sources for " + target.label;
  action.inputs = srcs_it->second;
  action.outputs = {classes_dir};

  action.command = {toolchain_.javac,
                    "-source",
                    toolchain_.source_version,
                    "-target",
                    toolchain_.target_version,
                    "-d",
                    classes_dir};

  if (!classpath.empty()) {
    std::string cp_str;
    for (size_t i = 0; i < classpath.size(); ++i) {
      if (i > 0) {
        cp_str += ":";
      }
      cp_str += classpath[i];
    }
    action.command.push_back("-cp");
    action.command.push_back(cp_str);
  }

  // Append extra javacopts
  const auto& opts_it = target.attrs.find("javacopts");
  if (opts_it != target.attrs.end()) {
    for (const auto& opt : opts_it->second) {
      action.command.push_back(opt);
    }
  }

  // Source files last
  for (const auto& src : srcs_it->second) {
    action.command.push_back(src);
  }

  action.env["HORCRUX_CLASSES_DIR"] = classes_dir;
  return action;
}

auto JavaAdapter::plan_jar_action(const AdapterTarget& target, const std::string& output_root) const
    -> tl::expected<BuildAction, AdapterError> {
  const std::string name = label_target_name(target.label);
  const std::string safe = label_to_safe_path(target.label);
  const std::string classes_dir = output_root + "/classes/" + safe;
  const std::string jar_path = output_root + "/jar/" + safe + "/" + name + ".jar";

  BuildAction action;
  action.kind = BuildAction::Kind::Archive;
  action.description = "Packaging JAR for " + target.label;
  action.inputs = {classes_dir};
  action.outputs = {jar_path};

  action.command = {toolchain_.jar_tool, "cf", jar_path, "-C", classes_dir, "."};

  // For java_binary, set the Main-Class manifest entry via -e
  const auto& main_it = target.attrs.find("main_class");
  if (main_it != target.attrs.end() && !main_it->second.empty() && target.kind == "java_binary") {
    action.command = {toolchain_.jar_tool, "cfe", jar_path, main_it->second[0], "-C",
                      classes_dir,         "."};
  }

  return action;
}

auto JavaAdapter::plan_test_action(const AdapterTarget& target, const std::string& output_root)
    const -> tl::expected<BuildAction, AdapterError> {
  const std::string name = label_target_name(target.label);
  const std::string safe = label_to_safe_path(target.label);
  const std::string jar_path = output_root + "/jar/" + safe + "/" + name + ".jar";

  const auto& main_it = target.attrs.find("main_class");
  std::string main_class = "org.junit.runner.JUnitCore";
  if (main_it != target.attrs.end() && !main_it->second.empty()) {
    main_class = main_it->second[0];
  }

  BuildAction action;
  action.kind = BuildAction::Kind::Test;
  action.description = "Running Java tests for " + target.label;
  action.inputs = {jar_path};
  action.outputs = {};
  action.command = {toolchain_.java, "-cp", jar_path, main_class};
  return action;
}

auto JavaAdapter::plan_actions(const AdapterTarget& target, const BuildGraph& graph,
                               const std::string& output_root)
    -> tl::expected<std::vector<BuildAction>, AdapterError> {
  diagnostics_.clear();

  std::vector<BuildAction> actions;

  const auto& srcs_it = target.attrs.find("srcs");
  const bool has_sources = srcs_it != target.attrs.end() && !srcs_it->second.empty();

  if (!has_sources) {
    return actions;
  }

  // Compile
  auto compile = plan_compile_action(target, graph, output_root);
  if (!compile) {
    return tl::unexpected(compile.error());
  }
  actions.push_back(std::move(*compile));

  // Archive as JAR
  auto jar = plan_jar_action(target, output_root);
  if (!jar) {
    return tl::unexpected(jar.error());
  }
  actions.push_back(std::move(*jar));

  // Test run action for java_test
  if (target.kind == "java_test") {
    auto test = plan_test_action(target, output_root);
    if (!test) {
      return tl::unexpected(test.error());
    }
    actions.push_back(std::move(*test));
  }

  return actions;
}

// ─────────────────────────────────────────────────────────────────────────────
// compute_cache_key
// ─────────────────────────────────────────────────────────────────────────────

auto JavaAdapter::compute_cache_key(const AdapterTarget& target)
    -> tl::expected<Hash, AdapterError> {
  std::string key_material;
  key_material += "java:";
  key_material += target.label;
  key_material += ":";
  key_material += target.kind;
  key_material += ":";
  key_material += toolchain_.java_version;
  key_material += ":";
  key_material += toolchain_.source_version;
  key_material += ":";

  const auto& srcs_it = target.attrs.find("srcs");
  if (srcs_it != target.attrs.end()) {
    auto sorted_srcs = srcs_it->second;
    std::sort(sorted_srcs.begin(), sorted_srcs.end());
    for (const auto& s : sorted_srcs) {
      key_material += s;
      key_material += ",";
    }
  }

  key_material += ":";
  const auto& opts_it = target.attrs.find("javacopts");
  if (opts_it != target.attrs.end()) {
    for (const auto& opt : opts_it->second) {
      key_material += opt;
      key_material += ",";
    }
  }

  const auto* data = reinterpret_cast<const uint8_t*>(key_material.data());
  return compute_sha256({data, key_material.size()});
}

} // namespace horcrux::core
