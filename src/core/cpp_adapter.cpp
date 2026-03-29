// Horcrux - C++ Adapter Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "cpp_adapter.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <sstream>

namespace horcrux::core {

namespace {

constexpr std::string_view kAdapterName = "cpp";
constexpr std::string_view kAdapterVersion = "1.0.0";

/// Candidate compiler executables in priority order
constexpr std::array<std::string_view, 4> kCompilerCandidates = {
    "g++", "clang++", "c++", "cc"
};

/// Minimum depth for an absolute path to be considered safe for removal
constexpr int MIN_SAFE_ABSOLUTE_PATH_DEPTH = 3;

/// C++ source file extensions considered for compilation
constexpr std::array<std::string_view, 4> kCppSourceExtensions = {
    ".cpp", ".cc", ".cxx", ".c"
};

/// Check if an executable exists on PATH using POSIX access()
auto executable_exists(std::string_view name) -> bool {
  // Check HORCRUX_CXX override first
  if (name == kCompilerCandidates[0]) {
    const char* env_cxx = std::getenv("HORCRUX_CXX");
    if (env_cxx != nullptr && std::filesystem::exists(env_cxx)) {
      return true;
    }
  }
  // Search PATH for the executable by trying to locate it
  const char* path_env = std::getenv("PATH");
  if (path_env == nullptr) {
    return false;
  }
  std::string path_str(path_env);
  std::istringstream path_stream(path_str);
  std::string dir;
  while (std::getline(path_stream, dir, ':')) {
    auto candidate = std::filesystem::path(dir) / name;
    if (std::filesystem::exists(candidate)) {
      return true;
    }
  }
  return false;
}

/// Resolve full path of a compiler on PATH
auto resolve_compiler(std::string_view name) -> std::string {
  // Check HORCRUX_CXX override
  const char* env_cxx = std::getenv("HORCRUX_CXX");
  if (env_cxx != nullptr && std::filesystem::exists(env_cxx)) {
    return std::string(env_cxx);
  }
  return std::string(name);
}

/// Detect compiler version string
auto detect_version(const std::string& compiler) -> std::string {
  // Return a placeholder version; real impl would run `compiler --version`
  if (compiler.find("clang") != std::string::npos) {
    return "clang-unknown";
  }
  return "gcc-unknown";
}

/// Derive archiver from compiler path (g++ → ar, clang++ → llvm-ar)
auto derive_archiver(const std::string& compiler) -> std::string {
  if (compiler.find("clang") != std::string::npos) {
    return "ar"; // llvm-ar if available; fall back to ar
  }
  return "ar";
}

/// Convert label "//pkg/sub:name" to a path-safe string "pkg_sub_name"
auto label_to_safe_path(const std::string& label) -> std::string {
  // Strip "//" prefix
  std::string result = label;
  if (result.size() >= 2 && result[0] == '/' && result[1] == '/') {
    result = result.substr(2);
  }
  // Replace '/' and ':' with '_'
  std::replace(result.begin(), result.end(), '/', '_');
  std::replace(result.begin(), result.end(), ':', '_');
  return result;
}

/// Get target name from label "//pkg:name" → "name"
auto extract_target_name(const std::string& label) -> std::string {
  auto colon = label.rfind(':');
  if (colon != std::string::npos) {
    return label.substr(colon + 1);
  }
  // Implicit name: last path component
  auto slash = label.rfind('/');
  if (slash != std::string::npos) {
    return label.substr(slash + 1);
  }
  return label;
}

/// Split a string by whitespace and/or commas
auto split_tokens(const std::string& value) -> std::vector<std::string> {
  std::vector<std::string> tokens;
  std::istringstream stream(value);
  std::string token;
  while (stream >> token) {
    // Strip trailing commas
    if (!token.empty() && token.back() == ',') {
      token.pop_back();
    }
    if (!token.empty()) {
      tokens.push_back(std::move(token));
    }
  }
  return tokens;
}

/// Return only .cpp/.cc/.cxx source files from a list
auto filter_sources(const std::vector<std::string>& files) -> std::vector<std::string> {
  std::vector<std::string> sources;
  for (const auto& f : files) {
    auto ext = std::filesystem::path(f).extension().string();
    for (const auto& valid_ext : kCppSourceExtensions) {
      if (ext == valid_ext) {
        sources.push_back(f);
        break;
      }
    }
  }
  return sources;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// CppAdapter static factory
// ─────────────────────────────────────────────────────────────────────────────

auto CppAdapter::create() -> tl::expected<CppAdapter, AdapterError> {
  auto tc = detect_toolchain();
  if (!tc) {
    return tl::unexpected(tc.error());
  }
  return CppAdapter{std::move(*tc)};
}

// ─────────────────────────────────────────────────────────────────────────────
// CppAdapter constructor
// ─────────────────────────────────────────────────────────────────────────────

CppAdapter::CppAdapter(CppToolchain toolchain)
    : toolchain_(std::move(toolchain)) {
  info_.name = std::string(kAdapterName);
  info_.version = std::string(kAdapterVersion);
  info_.supported_kinds = {"cc_library", "cc_binary", "cc_test"};
}

// ─────────────────────────────────────────────────────────────────────────────
// Adapter interface
// ─────────────────────────────────────────────────────────────────────────────

auto CppAdapter::info() const -> const AdapterInfo& {
  return info_;
}

auto CppAdapter::diagnostics() const -> const std::vector<Diagnostic>& {
  return diagnostics_;
}

void CppAdapter::emit(Diagnostic::Level level, std::string message,
                      std::optional<std::string> location) const {
  diagnostics_.push_back(Diagnostic{level, std::move(message), std::move(location)});
}

// ─────────────────────────────────────────────────────────────────────────────
// parse_target
// ─────────────────────────────────────────────────────────────────────────────

auto CppAdapter::parse_target(const BuildNode& node) -> tl::expected<AdapterTarget, AdapterError> {
  diagnostics_.clear();

  const auto& kind = node.node_type();
  if (!supports_kind(kind)) {
    emit(Diagnostic::Level::Error, "Unsupported target kind: " + kind);
    return tl::unexpected(AdapterError::UnsupportedKind);
  }

  AdapterTarget target;
  target.label = node.label();
  target.kind  = kind;

  // Map inputs → srcs (source + header files)
  for (const auto& inp : node.inputs()) {
    target.attrs["inputs"].push_back(inp);
  }

  // Map outputs
  for (const auto& out : node.outputs()) {
    target.attrs["outputs"].push_back(out);
  }

  // Import known attributes from node metadata
  const auto& raw_attrs = node.attributes();
  for (const auto& key : {"srcs", "hdrs", "copts", "includes", "deps", "linkopts"}) {
    auto it = raw_attrs.find(key);
    if (it != raw_attrs.end()) {
      target.attrs[key] = split_tokens(it->second);
    }
  }

  // Merge node inputs as srcs if srcs not explicitly provided
  if (target.attrs.find("srcs") == target.attrs.end() ||
      target.attrs.at("srcs").empty()) {
    auto sources = filter_sources(target.attrs["inputs"]);
    if (!sources.empty()) {
      target.attrs["srcs"] = std::move(sources);
    }
  }

  // Validate: cc_binary and cc_test must have at least one source
  if ((kind == "cc_binary" || kind == "cc_test") &&
      (target.attrs.find("srcs") == target.attrs.end() ||
       target.attrs.at("srcs").empty())) {
    emit(Diagnostic::Level::Warning,
         "Target " + target.label + " has no source files; build may be empty");
  }

  return target;
}

// ─────────────────────────────────────────────────────────────────────────────
// plan_actions
// ─────────────────────────────────────────────────────────────────────────────

auto CppAdapter::plan_actions(const AdapterTarget& target, const BuildGraph& graph,
                               const std::string& output_root)
    -> tl::expected<std::vector<BuildAction>, AdapterError> {
  diagnostics_.clear();
  std::vector<BuildAction> actions;
  std::vector<std::string> obj_files;

  // 1. Compile sources
  auto compile_result = plan_compile_actions(target, output_root, obj_files);
  if (!compile_result) {
    return tl::unexpected(compile_result.error());
  }
  for (auto& action : *compile_result) {
    actions.push_back(std::move(action));
  }

  // 2a. Archive (cc_library)
  if (target.kind == "cc_library") {
    if (!obj_files.empty()) {
      auto archive_result = plan_archive_action(target, output_root, obj_files);
      if (!archive_result) {
        return tl::unexpected(archive_result.error());
      }
      actions.push_back(std::move(*archive_result));
    }
    return actions;
  }

  // 2b. Link (cc_binary / cc_test)
  if (target.kind == "cc_binary" || target.kind == "cc_test") {
    auto link_result = plan_link_action(target, graph, output_root, obj_files);
    if (!link_result) {
      return tl::unexpected(link_result.error());
    }
    actions.push_back(std::move(*link_result));
    return actions;
  }

  emit(Diagnostic::Level::Error, "Cannot plan actions for unsupported kind: " + target.kind);
  return tl::unexpected(AdapterError::PlanningError);
}

// ─────────────────────────────────────────────────────────────────────────────
// plan_compile_actions
// ─────────────────────────────────────────────────────────────────────────────

auto CppAdapter::plan_compile_actions(const AdapterTarget& target,
                                       const std::string& output_root,
                                       std::vector<std::string>& obj_files) const
    -> tl::expected<std::vector<BuildAction>, AdapterError> {
  std::vector<BuildAction> actions;

  auto srcs_it = target.attrs.find("srcs");
  if (srcs_it == target.attrs.end() || srcs_it->second.empty()) {
    // No sources — nothing to compile (e.g., header-only library)
    return actions;
  }

  const auto& srcs = srcs_it->second;
  auto safe_label = label_to_safe_path(target.label);

  // Build include flags
  std::vector<std::string> include_flags;
  auto inc_it = target.attrs.find("includes");
  if (inc_it != target.attrs.end()) {
    for (const auto& inc : inc_it->second) {
      include_flags.push_back("-I" + inc);
    }
  }

  // Build extra compiler flags
  std::vector<std::string> copts;
  auto copts_it = target.attrs.find("copts");
  if (copts_it != target.attrs.end()) {
    copts = copts_it->second;
  }

  for (const auto& src : srcs) {
    auto src_path = std::filesystem::path(src);
    auto obj_name = safe_label + "__" + src_path.stem().string() + ".o";
    auto obj_path = std::filesystem::path(output_root) / obj_name;

    std::vector<std::string> cmd = {
        toolchain_.compiler, "-std=c++23", "-c", src,
        "-o", obj_path.string()
    };
    for (const auto& flag : include_flags) {
      cmd.push_back(flag);
    }
    for (const auto& flag : copts) {
      cmd.push_back(flag);
    }

    BuildAction action;
    action.kind = BuildAction::Kind::Compile;
    action.description = "Compiling " + src;
    action.inputs = {src};
    action.outputs = {obj_path.string()};
    action.command = std::move(cmd);

    obj_files.push_back(obj_path.string());
    actions.push_back(std::move(action));
  }

  return actions;
}

// ─────────────────────────────────────────────────────────────────────────────
// plan_archive_action
// ─────────────────────────────────────────────────────────────────────────────

auto CppAdapter::plan_archive_action(const AdapterTarget& target,
                                      const std::string& output_root,
                                      const std::vector<std::string>& obj_files) const
    -> tl::expected<BuildAction, AdapterError> {
  auto target_name = extract_target_name(target.label);
  auto lib_name = "lib" + target_name + ".a";
  auto lib_path = std::filesystem::path(output_root) / lib_name;

  BuildAction action;
  action.kind = BuildAction::Kind::Archive;
  action.description = "Archiving " + lib_path.string();
  action.inputs = obj_files;
  action.outputs = {lib_path.string()};

  // ar rcs libfoo.a foo.o bar.o
  action.command = {toolchain_.archiver, "rcs", lib_path.string()};
  for (const auto& obj : obj_files) {
    action.command.push_back(obj);
  }

  return action;
}

// ─────────────────────────────────────────────────────────────────────────────
// plan_link_action
// ─────────────────────────────────────────────────────────────────────────────

auto CppAdapter::plan_link_action(const AdapterTarget& target,
                                   const BuildGraph& graph,
                                   const std::string& output_root,
                                   const std::vector<std::string>& obj_files) const
    -> tl::expected<BuildAction, AdapterError> {
  auto target_name = extract_target_name(target.label);
  auto bin_path = std::filesystem::path(output_root) / target_name;

  BuildAction action;
  action.kind = BuildAction::Kind::Link;
  action.description = "Linking " + bin_path.string();
  action.outputs = {bin_path.string()};

  action.command = {toolchain_.compiler, "-std=c++23"};
  for (const auto& obj : obj_files) {
    action.command.push_back(obj);
    action.inputs.push_back(obj);
  }

  // Link against static libraries from dependency cc_library targets
  auto deps_it = target.attrs.find("deps");
  if (deps_it != target.attrs.end()) {
    for (const auto& dep_label : deps_it->second) {
      auto dep_name = extract_target_name(dep_label);
      auto dep_lib = std::filesystem::path(output_root) / ("lib" + dep_name + ".a");
      action.command.push_back(dep_lib.string());
      action.inputs.push_back(dep_lib.string());
    }
  }

  // Also resolve transitive dependencies from the build graph
  auto trans_deps = graph.get_transitive_dependencies(target.label);
  if (trans_deps.has_value()) {
    for (const auto& dep_label : *trans_deps) {
      auto* dep_node = graph.get_node(dep_label);
      if (dep_node && dep_node->node_type() == "cc_library") {
        auto dep_name = extract_target_name(dep_label);
        auto dep_lib = std::filesystem::path(output_root) / ("lib" + dep_name + ".a");
        // Avoid duplicates
        if (std::find(action.command.begin(), action.command.end(), dep_lib.string()) ==
            action.command.end()) {
          action.command.push_back(dep_lib.string());
          action.inputs.push_back(dep_lib.string());
        }
      }
    }
  }

  // Extra linker flags
  auto linkopts_it = target.attrs.find("linkopts");
  if (linkopts_it != target.attrs.end()) {
    for (const auto& opt : linkopts_it->second) {
      action.command.push_back(opt);
    }
  }

  action.command.push_back("-o");
  action.command.push_back(bin_path.string());

  return action;
}

// ─────────────────────────────────────────────────────────────────────────────
// compute_cache_key
// ─────────────────────────────────────────────────────────────────────────────

auto CppAdapter::compute_cache_key(const AdapterTarget& target)
    -> tl::expected<Hash, AdapterError> {
  // Build a deterministic string from normalized inputs
  std::string key_data;
  key_data += "adapter=cpp\n";
  key_data += "compiler=" + toolchain_.compiler_id + "-" + toolchain_.compiler_version + "\n";
  key_data += "label=" + target.label + "\n";
  key_data += "kind=" + target.kind + "\n";

  // Append sorted attribute keys and values for determinism
  std::vector<std::string> attr_keys;
  for (const auto& [k, _unused] : target.attrs) {
    attr_keys.push_back(k);
  }
  std::sort(attr_keys.begin(), attr_keys.end());

  for (const auto& key : attr_keys) {
    const auto& values = target.attrs.at(key);
    auto sorted_values = values;
    std::sort(sorted_values.begin(), sorted_values.end());
    key_data += key + "=";
    for (const auto& v : sorted_values) {
      key_data += v + ",";
    }
    key_data += "\n";
  }

  std::vector<uint8_t> bytes(key_data.begin(), key_data.end());
  return compute_sha256(bytes);
}

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

auto CppAdapter::detect_toolchain() -> tl::expected<CppToolchain, AdapterError> {
  // Check HORCRUX_CXX environment variable first
  const char* env_cxx = std::getenv("HORCRUX_CXX");
  if (env_cxx != nullptr) {
    std::string compiler_path(env_cxx);
    if (!compiler_path.empty()) {
      CppToolchain tc;
      tc.compiler = compiler_path;
      tc.compiler_id = "unknown";
      tc.compiler_version = detect_version(compiler_path);
      tc.archiver = derive_archiver(compiler_path);
      return tc;
    }
  }

  // Search PATH for known compiler names
  for (const auto& candidate : kCompilerCandidates) {
    if (executable_exists(candidate)) {
      CppToolchain tc;
      tc.compiler = resolve_compiler(candidate);
      tc.compiler_id =
          (std::string(candidate).find("clang") != std::string::npos) ? "clang" : "gcc";
      tc.compiler_version = detect_version(tc.compiler);
      tc.archiver = derive_archiver(tc.compiler);
      return tc;
    }
  }

  return tl::unexpected(AdapterError::ToolchainError);
}

auto CppAdapter::label_to_path(const std::string& label) -> std::string {
  return label_to_safe_path(label);
}

auto CppAdapter::label_target_name(const std::string& label) -> std::string {
  return extract_target_name(label);
}

auto CppAdapter::split_attr(const std::string& value) -> std::vector<std::string> {
  return split_tokens(value);
}

} // namespace horcrux::core
