// Horcrux - Python Adapter Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "python_adapter.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <sstream>

#include "local_cache.h"

namespace horcrux::core {

namespace {

constexpr std::string_view kAdapterName = "python";
constexpr std::string_view kAdapterVersion = "1.0.0";

/// Python source and package file extensions
constexpr std::array<std::string_view, 2> kPythonSourceExtensions = {".py", ".pyi"};

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

auto PythonAdapter::label_target_name(const std::string& label) -> std::string {
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

auto PythonAdapter::label_to_safe_path(const std::string& label) -> std::string {
  std::string result = label;
  if (result.size() >= 2 && result[0] == '/' && result[1] == '/') {
    result = result.substr(2);
  }
  std::replace(result.begin(), result.end(), '/', '_');
  std::replace(result.begin(), result.end(), ':', '_');
  return result;
}

auto PythonAdapter::split_tokens(const std::string& value) -> std::vector<std::string> {
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

auto PythonAdapter::filter_python_sources(const std::vector<std::string>& files)
    -> std::vector<std::string> {
  std::vector<std::string> sources;
  for (const auto& f : files) {
    auto ext = std::filesystem::path(f).extension().string();
    for (const auto& valid_ext : kPythonSourceExtensions) {
      if (ext == valid_ext) {
        sources.push_back(f);
        break;
      }
    }
  }
  return sources;
}

// ─────────────────────────────────────────────────────────────────────────────
// Toolchain detection
// ─────────────────────────────────────────────────────────────────────────────

auto PythonAdapter::detect_toolchain() -> tl::expected<PythonToolchain, AdapterError> {
  PythonToolchain tc;

  const char* env_python = std::getenv("HORCRUX_PYTHON");
  if (env_python != nullptr && std::filesystem::exists(env_python)) {
    tc.python = std::string(env_python);
  } else if (executable_on_path("python3")) {
    tc.python = "python3";
  } else if (executable_on_path("python")) {
    tc.python = "python";
  } else {
    return tl::unexpected(AdapterError::ToolchainError);
  }

  tc.python_version = "python-unknown";
  return tc;
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory and constructor
// ─────────────────────────────────────────────────────────────────────────────

auto PythonAdapter::create() -> tl::expected<PythonAdapter, AdapterError> {
  auto tc = detect_toolchain();
  if (!tc) {
    return tl::unexpected(tc.error());
  }
  return PythonAdapter{std::move(*tc)};
}

PythonAdapter::PythonAdapter(PythonToolchain toolchain) : toolchain_(std::move(toolchain)) {
  info_.name = std::string(kAdapterName);
  info_.version = std::string(kAdapterVersion);
  info_.supported_kinds = {"py_library", "py_binary", "py_test"};
}

// ─────────────────────────────────────────────────────────────────────────────
// Adapter interface
// ─────────────────────────────────────────────────────────────────────────────

auto PythonAdapter::info() const -> const AdapterInfo& {
  return info_;
}

auto PythonAdapter::diagnostics() const -> const std::vector<Diagnostic>& {
  return diagnostics_;
}

void PythonAdapter::emit(Diagnostic::Level level, std::string message,
                         std::optional<std::string> location) const {
  diagnostics_.push_back(Diagnostic{level, std::move(message), std::move(location)});
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_target
// ─────────────────────────────────────────────────────────────────────────────

auto PythonAdapter::parse_target(const BuildNode& node)
    -> tl::expected<AdapterTarget, AdapterError> {
  diagnostics_.clear();

  const auto& kind = node.node_type();
  if (!supports_kind(kind)) {
    emit(Diagnostic::Level::Error, "Unsupported target kind: " + kind);
    return tl::unexpected(AdapterError::UnsupportedKind);
  }

  AdapterTarget target;
  target.label = node.label();
  target.kind = kind;

  // Populate srcs from node inputs (.py files)
  target.attrs["srcs"] = filter_python_sources(node.inputs());

  // All source inputs (not just .py) may include data files
  target.attrs["data"] = node.inputs();

  // Deps
  auto deps_attr = node.get_attribute("deps");
  if (deps_attr) {
    target.attrs["deps"] = split_tokens(*deps_attr);
  }

  // Main module for py_binary / py_test
  auto main_attr = node.get_attribute("main");
  if (main_attr) {
    target.attrs["main"] = {*main_attr};
  } else if (!target.attrs["srcs"].empty()) {
    target.attrs["main"] = {target.attrs["srcs"][0]};
  }

  return target;
}

// ─────────────────────────────────────────────────────────────────────────────
// plan_actions
// ─────────────────────────────────────────────────────────────────────────────

auto PythonAdapter::plan_package_action(const AdapterTarget& target, const std::string& output_root)
    const -> tl::expected<BuildAction, AdapterError> {
  const std::string name = label_target_name(target.label);
  const std::string safe = label_to_safe_path(target.label);
  const std::string pkg_dir = output_root + "/py/" + safe;

  const auto& srcs_it = target.attrs.find("srcs");
  if (srcs_it == target.attrs.end() || srcs_it->second.empty()) {
    emit(Diagnostic::Level::Error, "py target has no .py sources: " + target.label);
    return tl::unexpected(AdapterError::PlanningError);
  }

  BuildAction action;
  action.kind = BuildAction::Kind::Custom;
  action.description = "Packaging Python sources for " + target.label;
  action.inputs = srcs_it->second;
  action.outputs = {pkg_dir + "/__init__.py"};

  // Use python -m py_compile to bytecode-check all sources
  action.command = {toolchain_.python, "-m", "py_compile"};
  for (const auto& src : srcs_it->second) {
    action.command.push_back(src);
  }

  action.env["PYTHONDONTWRITEBYTECODE"] = "1";
  return action;
}

auto PythonAdapter::plan_test_action(const AdapterTarget& target, const std::string& output_root)
    const -> tl::expected<BuildAction, AdapterError> {
  const auto& main_it = target.attrs.find("main");
  std::string main_module;
  if (main_it != target.attrs.end() && !main_it->second.empty()) {
    main_module = main_it->second[0];
  } else {
    const auto& srcs_it = target.attrs.find("srcs");
    if (srcs_it != target.attrs.end() && !srcs_it->second.empty()) {
      main_module = srcs_it->second[0];
    }
  }

  if (main_module.empty()) {
    emit(Diagnostic::Level::Error, "py_test has no main module: " + target.label);
    return tl::unexpected(AdapterError::PlanningError);
  }

  const std::string safe = label_to_safe_path(target.label);

  BuildAction action;
  action.kind = BuildAction::Kind::Test;
  action.description = "Running Python tests for " + target.label;
  action.inputs = {main_module};
  action.outputs = {};
  action.command = {toolchain_.python, "-m", "unittest", main_module};
  action.env["HORCRUX_TEST_ROOT"] = output_root + "/test/" + safe;
  return action;
}

auto PythonAdapter::plan_actions(const AdapterTarget& target, const BuildGraph& /*graph*/,
                                 const std::string& output_root)
    -> tl::expected<std::vector<BuildAction>, AdapterError> {
  diagnostics_.clear();

  std::vector<BuildAction> actions;

  const auto& srcs_it = target.attrs.find("srcs");
  const bool has_sources = srcs_it != target.attrs.end() && !srcs_it->second.empty();

  if (!has_sources) {
    return actions;
  }

  if (target.kind == "py_library" || target.kind == "py_binary") {
    auto pkg = plan_package_action(target, output_root);
    if (!pkg) {
      return tl::unexpected(pkg.error());
    }
    actions.push_back(std::move(*pkg));
  } else if (target.kind == "py_test") {
    auto pkg = plan_package_action(target, output_root);
    if (!pkg) {
      return tl::unexpected(pkg.error());
    }
    actions.push_back(std::move(*pkg));

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

auto PythonAdapter::compute_cache_key(const AdapterTarget& target)
    -> tl::expected<Hash, AdapterError> {
  std::string key_material;
  key_material += "python:";
  key_material += target.label;
  key_material += ":";
  key_material += target.kind;
  key_material += ":";
  key_material += toolchain_.python_version;
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

  const auto* data = reinterpret_cast<const uint8_t*>(key_material.data());
  return compute_sha256({data, key_material.size()});
}

} // namespace horcrux::core
