// Horcrux - Rust Adapter Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "rust_adapter.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <sstream>

#include "local_cache.h"

namespace horcrux::core {

namespace {

constexpr std::string_view kAdapterName = "rust";
constexpr std::string_view kAdapterVersion = "1.0.0";
constexpr std::string_view kDefaultEdition = "2021";

/// Rust source file extensions
constexpr std::array<std::string_view, 1> kRustSourceExtensions = {".rs"};

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

auto RustAdapter::label_target_name(const std::string& label) -> std::string {
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

auto RustAdapter::label_to_safe_path(const std::string& label) -> std::string {
  std::string result = label;
  if (result.size() >= 2 && result[0] == '/' && result[1] == '/') {
    result = result.substr(2);
  }
  std::replace(result.begin(), result.end(), '/', '_');
  std::replace(result.begin(), result.end(), ':', '_');
  return result;
}

auto RustAdapter::split_tokens(const std::string& value) -> std::vector<std::string> {
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

auto RustAdapter::filter_rust_sources(const std::vector<std::string>& files)
    -> std::vector<std::string> {
  std::vector<std::string> sources;
  for (const auto& f : files) {
    auto ext = std::filesystem::path(f).extension().string();
    for (const auto& valid_ext : kRustSourceExtensions) {
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

auto RustAdapter::detect_toolchain() -> tl::expected<RustToolchain, AdapterError> {
  RustToolchain tc;

  // Honor HORCRUX_RUSTC override
  const char* env_rustc = std::getenv("HORCRUX_RUSTC");
  if (env_rustc != nullptr && std::filesystem::exists(env_rustc)) {
    tc.rustc = std::string(env_rustc);
  } else if (executable_on_path("rustc")) {
    tc.rustc = "rustc";
  } else {
    return tl::unexpected(AdapterError::ToolchainError);
  }

  tc.cargo = executable_on_path("cargo") ? "cargo" : "";
  tc.archiver = "ar";
  tc.rustc_version = "rustc-unknown";
  tc.edition = std::string(kDefaultEdition);
  return tc;
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory and constructor
// ─────────────────────────────────────────────────────────────────────────────

auto RustAdapter::create() -> tl::expected<RustAdapter, AdapterError> {
  auto tc = detect_toolchain();
  if (!tc) {
    return tl::unexpected(tc.error());
  }
  return RustAdapter{std::move(*tc)};
}

RustAdapter::RustAdapter(RustToolchain toolchain) : toolchain_(std::move(toolchain)) {
  info_.name = std::string(kAdapterName);
  info_.version = std::string(kAdapterVersion);
  info_.supported_kinds = {"rust_library", "rust_binary", "rust_test"};
}

// ─────────────────────────────────────────────────────────────────────────────
// Adapter interface
// ─────────────────────────────────────────────────────────────────────────────

auto RustAdapter::info() const -> const AdapterInfo& {
  return info_;
}

auto RustAdapter::diagnostics() const -> const std::vector<Diagnostic>& {
  return diagnostics_;
}

void RustAdapter::emit(Diagnostic::Level level, std::string message,
                       std::optional<std::string> location) const {
  diagnostics_.push_back(Diagnostic{level, std::move(message), std::move(location)});
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_target
// ─────────────────────────────────────────────────────────────────────────────

auto RustAdapter::parse_target(const BuildNode& node) -> tl::expected<AdapterTarget, AdapterError> {
  diagnostics_.clear();

  const auto& kind = node.node_type();
  if (!supports_kind(kind)) {
    emit(Diagnostic::Level::Error, "Unsupported target kind: " + kind);
    return tl::unexpected(AdapterError::UnsupportedKind);
  }

  AdapterTarget target;
  target.label = node.label();
  target.kind = kind;

  // Populate srcs from node inputs (filter .rs files)
  auto sources = filter_rust_sources(node.inputs());
  target.attrs["srcs"] = sources;

  // Populate deps from node attributes
  auto deps_attr = node.get_attribute("deps");
  if (deps_attr) {
    target.attrs["deps"] = split_tokens(*deps_attr);
  }

  // Edition
  auto edition_attr = node.get_attribute("edition");
  target.attrs["edition"] = {edition_attr.value_or(toolchain_.edition)};

  // rustc_flags
  auto flags_attr = node.get_attribute("rustc_flags");
  if (flags_attr) {
    target.attrs["rustc_flags"] = split_tokens(*flags_attr);
  }

  return target;
}

// ─────────────────────────────────────────────────────────────────────────────
// plan_actions
// ─────────────────────────────────────────────────────────────────────────────

auto RustAdapter::plan_compile_action(const AdapterTarget& target, const std::string& output_root)
    const -> tl::expected<BuildAction, AdapterError> {
  const auto& srcs_it = target.attrs.find("srcs");
  if (srcs_it == target.attrs.end() || srcs_it->second.empty()) {
    emit(Diagnostic::Level::Error, "rust target has no .rs sources: " + target.label);
    return tl::unexpected(AdapterError::PlanningError);
  }

  const auto& srcs = srcs_it->second;
  const std::string name = label_target_name(target.label);
  const std::string safe = label_to_safe_path(target.label);
  const std::string obj_dir = output_root + "/objs/" + safe;

  BuildAction action;
  action.kind = BuildAction::Kind::Compile;
  action.description = "Compiling Rust crate " + target.label;
  action.inputs = srcs;

  const auto& edition_it = target.attrs.find("edition");
  const std::string edition = (edition_it != target.attrs.end() && !edition_it->second.empty())
                                  ? edition_it->second[0]
                                  : toolchain_.edition;

  // rustc --edition <edition> --crate-name <name> --crate-type <type>
  //       -o <output> <src>
  std::string crate_type;
  std::string out_path;
  if (target.kind == "rust_library") {
    crate_type = "rlib";
    out_path = output_root + "/lib/" + safe + "/lib" + name + ".rlib";
  } else if (target.kind == "rust_binary") {
    crate_type = "bin";
    out_path = output_root + "/bin/" + safe + "/" + name;
  } else {
    // rust_test → compile as test binary
    crate_type = "bin";
    out_path = output_root + "/test/" + safe + "/" + name + "_test";
  }

  action.outputs = {out_path};
  action.command = {toolchain_.rustc, "--edition", edition, "--crate-name", name,
                    "--crate-type",   crate_type,  "-o",    out_path,       srcs[0]};

  // Append extra rustc_flags
  const auto& flags_it = target.attrs.find("rustc_flags");
  if (flags_it != target.attrs.end()) {
    for (const auto& flag : flags_it->second) {
      action.command.push_back(flag);
    }
  }

  if (target.kind == "rust_test") {
    action.command.push_back("--test");
  }

  action.env["HORCRUX_OBJ_DIR"] = obj_dir;
  return action;
}

auto RustAdapter::plan_link_action(const AdapterTarget& target, const BuildGraph& graph,
                                   const std::string& output_root) const
    -> tl::expected<BuildAction, AdapterError> {
  const std::string name = label_target_name(target.label);
  const std::string safe = label_to_safe_path(target.label);
  const std::string bin_path = output_root + "/bin/" + safe + "/" + name;

  BuildAction action;
  action.kind = BuildAction::Kind::Link;
  action.description = "Linking Rust binary " + target.label;
  action.outputs = {bin_path};

  // Collect rlib deps from graph
  auto deps = graph.get_dependencies(target.label);
  std::vector<std::string> rlib_inputs;
  if (deps) {
    for (const auto& dep_label : *deps) {
      const std::string dep_safe = label_to_safe_path(dep_label);
      const std::string dep_name = label_target_name(dep_label);
      rlib_inputs.push_back(output_root + "/lib/" + dep_safe + "/lib" + dep_name + ".rlib");
    }
  }

  action.inputs = rlib_inputs;
  action.command = {toolchain_.rustc, "--edition", toolchain_.edition, "--crate-type", "bin", "-o",
                    bin_path};
  for (const auto& rlib : rlib_inputs) {
    action.command.push_back("--extern");
    action.command.push_back(rlib);
  }

  return action;
}

auto RustAdapter::plan_test_action(const AdapterTarget& target, const std::string& output_root)
    const -> tl::expected<BuildAction, AdapterError> {
  const std::string name = label_target_name(target.label);
  const std::string safe = label_to_safe_path(target.label);
  const std::string test_bin = output_root + "/test/" + safe + "/" + name + "_test";

  BuildAction action;
  action.kind = BuildAction::Kind::Test;
  action.description = "Running Rust tests for " + target.label;
  action.inputs = {test_bin};
  action.outputs = {};
  action.command = {test_bin};
  return action;
}

auto RustAdapter::plan_actions(const AdapterTarget& target, const BuildGraph& graph,
                               const std::string& output_root)
    -> tl::expected<std::vector<BuildAction>, AdapterError> {
  diagnostics_.clear();

  std::vector<BuildAction> actions;

  const auto& srcs_it = target.attrs.find("srcs");
  const bool has_sources = srcs_it != target.attrs.end() && !srcs_it->second.empty();

  if (!has_sources) {
    // No sources — e.g., re-export library with no own .rs files
    return actions;
  }

  auto compile = plan_compile_action(target, output_root);
  if (!compile) {
    return tl::unexpected(compile.error());
  }
  actions.push_back(std::move(*compile));

  if (target.kind == "rust_test") {
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

auto RustAdapter::compute_cache_key(const AdapterTarget& target)
    -> tl::expected<Hash, AdapterError> {
  // Build a deterministic string from all inputs that affect the output
  std::string key_material;
  key_material += "rust:";
  key_material += target.label;
  key_material += ":";
  key_material += target.kind;
  key_material += ":";
  key_material += toolchain_.rustc_version;
  key_material += ":";

  // Sorted srcs for determinism
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
  const auto& flags_it = target.attrs.find("rustc_flags");
  if (flags_it != target.attrs.end()) {
    for (const auto& f : flags_it->second) {
      key_material += f;
      key_material += ",";
    }
  }

  const auto* data = reinterpret_cast<const uint8_t*>(key_material.data());
  return compute_sha256({data, key_material.size()});
}

} // namespace horcrux::core
