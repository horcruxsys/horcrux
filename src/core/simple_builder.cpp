// Horcrux - Simple Builder Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "simple_builder.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_set>

namespace fs = std::filesystem;

namespace horcrux::core {

auto to_string(BuildError error) -> std::string {
  switch (error) {
  case BuildError::InvalidTarget:
    return "Invalid target format";
  case BuildError::CompilationFailed:
    return "Compilation failed";
  case BuildError::SourceNotFound:
    return "Source file not found";
  case BuildError::BuildFileNotFound:
    return "BUILD file not found";
  case BuildError::CacheError:
    return "Cache operation failed";
  }
  return "Unknown error";
}

SimpleBuilder::SimpleBuilder() {
  auto cache_result = LocalCache::create(".horcrux-cache");
  if (cache_result) {
    build_cache_ = std::move(*cache_result);
  }
}

// ---------------------------------------------------------------------------
// BUILD file parsing helpers
// ---------------------------------------------------------------------------

namespace {

/// Extract a quoted string attribute: attr_name = "value"
auto extract_string_attr(const std::string& block, const std::string& attr_name) -> std::string {
  std::regex re(attr_name + R"re(\s*=\s*"([^"]*)")re");
  std::smatch m;
  if (std::regex_search(block, m, re)) {
    return m[1].str();
  }
  return {};
}

/// Extract a list attribute: attr_name = ["item1", "item2", ...]
auto extract_string_list(const std::string& block,
                         const std::string& attr_name) -> std::vector<std::string> {
  std::vector<std::string> result;
  // Match attr_name = [ ... ]  (list may span lines, handled by [\s\S])
  std::regex list_re(attr_name + R"(\s*=\s*\[([^\]]*)\])");
  std::smatch m;
  if (std::regex_search(block, m, list_re)) {
    std::string list_str = m[1].str();
    std::regex item_re(R"re("([^"]*)")re");
    for (auto it = std::sregex_iterator(list_str.begin(), list_str.end(), item_re);
         it != std::sregex_iterator(); ++it) {
      result.push_back((*it)[1].str());
    }
  }
  return result;
}

/// Known rule type keywords (order matters: longest first to avoid prefix matches)
constexpr std::array known_rules{
    "cc_binary", "cc_library", "cc_test", "java_binary", "java_library", "java_test",
    "py_binary", "py_library", "py_test", "rust_binary", "rust_library", "rust_test",
};

} // anonymous namespace

auto SimpleBuilder::parse_all_targets(const fs::path& build_file) -> std::vector<ParsedTarget> {
  std::ifstream f(build_file);
  if (!f)
    return {};

  std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  std::vector<ParsedTarget> targets;

  size_t pos = 0;
  while (pos < content.size()) {
    // Find the earliest known rule keyword followed by '('
    size_t best_pos = std::string::npos;
    std::string best_rule;
    for (const auto* rule : known_rules) {
      std::string needle{rule};
      needle += '(';
      size_t found = content.find(needle, pos);
      // Make sure it's not mid-identifier (preceded by alnum or _)
      if (found != std::string::npos) {
        if (found > 0 && (std::isalnum(static_cast<unsigned char>(content[found - 1])) ||
                          content[found - 1] == '_')) {
          continue;
        }
        if (found < best_pos) {
          best_pos = found;
          best_rule = rule;
        }
      }
    }

    if (best_pos == std::string::npos)
      break;

    size_t open_paren = content.find('(', best_pos);
    if (open_paren == std::string::npos)
      break;

    // Find the matching closing parenthesis
    int depth = 1;
    size_t i = open_paren + 1;
    while (i < content.size() && depth > 0) {
      if (content[i] == '(')
        ++depth;
      else if (content[i] == ')')
        --depth;
      ++i;
    }

    std::string block = content.substr(open_paren + 1, i - open_paren - 2);

    ParsedTarget t;
    t.rule_type = best_rule;
    t.name = extract_string_attr(block, "name");
    t.srcs = extract_string_list(block, "srcs");
    t.deps = extract_string_list(block, "deps");
    t.main_class = extract_string_attr(block, "main_class");
    t.main_file = extract_string_attr(block, "main");
    t.edition = extract_string_attr(block, "edition");

    if (!t.name.empty()) {
      targets.push_back(std::move(t));
    }
    pos = i;
  }
  return targets;
}

auto SimpleBuilder::collect_all_sources(const ParsedTarget& target,
                                        const std::vector<ParsedTarget>& all_targets,
                                        const fs::path& package_dir) -> std::vector<fs::path> {
  std::vector<fs::path> result;
  std::unordered_set<std::string> visited;

  std::function<void(const ParsedTarget&)> collect = [&](const ParsedTarget& t) {
    if (visited.count(t.name))
      return;
    visited.insert(t.name);

    // Recurse into local package deps first (so libs are added before binaries)
    for (const auto& dep : t.deps) {
      if (dep.size() > 1 && dep[0] == ':') {
        std::string dep_name = dep.substr(1);
        auto it = std::find_if(all_targets.begin(), all_targets.end(),
                               [&](const ParsedTarget& pt) { return pt.name == dep_name; });
        if (it != all_targets.end()) {
          collect(*it);
        }
      }
    }

    for (const auto& src : t.srcs) {
      result.push_back(package_dir / src);
    }
  };

  collect(target);
  return result;
}

// ---------------------------------------------------------------------------
// Parse target label
// ---------------------------------------------------------------------------

auto SimpleBuilder::parse_target(std::string_view target) -> tl::expected<TargetInfo, BuildError> {
  std::regex target_regex(R"(^//([^:]+):([^:]+)$)");
  std::smatch matches;
  std::string target_str(target);
  if (!std::regex_match(target_str, matches, target_regex)) {
    return tl::unexpected(BuildError::InvalidTarget);
  }
  TargetInfo info;
  info.package_path = matches[1].str();
  info.target_name = matches[2].str();
  return info;
}

auto SimpleBuilder::build_file_exists(std::string_view package_path) -> bool {
  return fs::exists(fs::path(package_path) / "BUILD");
}

auto SimpleBuilder::get_compiler() -> const std::string& {
  if (!cached_compiler_) {
    if (std::system("which g++ > /dev/null 2>&1") == 0) {
      cached_compiler_ = "g++";
    } else if (std::system("which clang++ > /dev/null 2>&1") == 0) {
      cached_compiler_ = "clang++";
    } else {
      cached_compiler_ = "c++";
    }
  }
  return *cached_compiler_;
}

auto SimpleBuilder::source_changed(const fs::path& source_file,
                                   const fs::path& output_binary) -> bool {
  if (!fs::exists(output_binary))
    return true;
  return fs::last_write_time(source_file) > fs::last_write_time(output_binary);
}

// ---------------------------------------------------------------------------
// cc_binary builder
// ---------------------------------------------------------------------------

auto SimpleBuilder::compile_cc_binary(const TargetInfo& info, const ParsedTarget& target)
    -> tl::expected<void, BuildError> {
  auto start = std::chrono::steady_clock::now();

  fs::path package_dir(info.package_path);
  fs::path output_dir = fs::path("bazel-bin") / info.package_path;
  fs::create_directories(output_dir);
  fs::path output_binary = output_dir / info.target_name;

  // Collect source files
  std::vector<fs::path> sources;
  for (const auto& src : target.srcs) {
    sources.push_back(package_dir / src);
  }

  if (sources.empty()) {
    // Fallback: look for main.cpp (legacy behaviour)
    fs::path fallback = package_dir / "main.cpp";
    if (!fs::exists(fallback)) {
      std::cerr << "Error: Source file not found: \"" << (package_dir / "main.cpp").string()
                << "\"\n";
      return tl::unexpected(BuildError::SourceNotFound);
    }
    sources.push_back(fallback);
  }

  // Incremental check: if output is newer than all sources, skip
  if (fs::exists(output_binary)) {
    bool needs_rebuild = false;
    for (const auto& src : sources) {
      if (!fs::exists(src)) {
        std::cerr << "Error: Source file not found: " << src << "\n";
        return tl::unexpected(BuildError::SourceNotFound);
      }
      if (source_changed(src, output_binary)) {
        needs_rebuild = true;
        break;
      }
    }
    if (!needs_rebuild) {
      auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - start);
      std::cout << "✓ Target up-to-date: " << output_binary << " (" << dur.count()
                << "ms, incremental)\n";
      return {};
    }
  }

  const auto& compiler = get_compiler();
  std::string cmd = compiler + " -std=c++23 -O2 -Wall";
  for (const auto& src : sources) {
    if (!fs::exists(src)) {
      std::cerr << "Error: Source file not found: " << src << "\n";
      return tl::unexpected(BuildError::SourceNotFound);
    }
    cmd += " " + src.string();
  }
  cmd += " -o " + output_binary.string();

  std::cout << "Compiling [C++]: " << info.package_path << ":" << info.target_name << "\n";
  if (std::system(cmd.c_str()) != 0) {
    return tl::unexpected(BuildError::CompilationFailed);
  }

  auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);
  std::cout << "✓ Build successful: " << output_binary << " (" << dur.count() << "ms)\n";
  return {};
}

// ---------------------------------------------------------------------------
// java_binary builder
// ---------------------------------------------------------------------------

auto SimpleBuilder::build_java_binary(const TargetInfo& info, const ParsedTarget& target,
                                      const std::vector<ParsedTarget>& all_targets)
    -> tl::expected<void, BuildError> {
  auto start = std::chrono::steady_clock::now();

  fs::path package_dir(info.package_path);
  fs::path classes_dir = fs::path("bazel-bin") / info.package_path / "classes";
  fs::path output_dir = fs::path("bazel-bin") / info.package_path;
  fs::create_directories(classes_dir);

  // Collect all java sources (deps first, then own srcs)
  auto all_sources = collect_all_sources(target, all_targets, package_dir);

  if (all_sources.empty()) {
    std::cerr << "Error: No source files found for target " << info.target_name << "\n";
    return tl::unexpected(BuildError::SourceNotFound);
  }

  for (const auto& src : all_sources) {
    if (!fs::exists(src)) {
      std::cerr << "Error: Source file not found: " << src << "\n";
      return tl::unexpected(BuildError::SourceNotFound);
    }
  }

  // Build javac command
  std::string javac_cmd = "javac -d " + classes_dir.string();
  for (const auto& src : all_sources) {
    javac_cmd += " " + src.string();
  }
  javac_cmd += " 2>&1";

  std::cout << "Compiling [Java]: " << info.package_path << ":" << info.target_name << "\n";
  for (const auto& src : all_sources) {
    std::cout << "  " << src.string() << "\n";
  }

  if (std::system(javac_cmd.c_str()) != 0) {
    return tl::unexpected(BuildError::CompilationFailed);
  }

  // Write a launcher shell script
  fs::path launcher = output_dir / info.target_name;
  {
    std::ofstream script(launcher);
    if (!script) {
      std::cerr << "Error: Cannot write launcher script: " << launcher << "\n";
      return tl::unexpected(BuildError::CompilationFailed);
    }
    // Use absolute path to classes dir so the script works from any cwd
    std::string abs_classes = fs::absolute(classes_dir).string();
    script << "#!/bin/bash\n";
    script << "exec java -cp \"" << abs_classes << "\" " << target.main_class << " \"$@\"\n";
  }
  fs::permissions(launcher,
                  fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec |
                      fs::perms::owner_read | fs::perms::owner_write,
                  fs::perm_options::add);

  auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);
  std::cout << "✓ Build successful: " << launcher << " (" << dur.count() << "ms)\n";
  return {};
}

// ---------------------------------------------------------------------------
// py_binary builder
// ---------------------------------------------------------------------------

auto SimpleBuilder::build_py_binary(const TargetInfo& info,
                                    const ParsedTarget& target) -> tl::expected<void, BuildError> {
  auto start = std::chrono::steady_clock::now();

  fs::path package_dir(info.package_path);
  fs::path output_dir = fs::path("bazel-bin") / info.package_path;
  fs::create_directories(output_dir);

  // Determine the main script
  std::string main_rel = target.main_file;
  if (main_rel.empty() && !target.srcs.empty()) {
    main_rel = target.srcs[0];
  }
  if (main_rel.empty()) {
    std::cerr << "Error: No main script found for py_binary " << info.target_name << "\n";
    return tl::unexpected(BuildError::SourceNotFound);
  }

  fs::path main_script = package_dir / main_rel;
  if (!fs::exists(main_script)) {
    std::cerr << "Error: Source file not found: " << main_script << "\n";
    return tl::unexpected(BuildError::SourceNotFound);
  }

  // The PYTHONPATH needs the src directory so that `from greet import greet` works
  std::string abs_src_dir = fs::absolute(package_dir / "src").string();
  // Also add the package dir itself in case some scripts live there
  std::string abs_pkg_dir = fs::absolute(package_dir).string();
  std::string abs_main = fs::absolute(main_script).string();

  fs::path launcher = output_dir / info.target_name;
  {
    std::ofstream script(launcher);
    if (!script) {
      std::cerr << "Error: Cannot write launcher script: " << launcher << "\n";
      return tl::unexpected(BuildError::CompilationFailed);
    }
    script << "#!/bin/bash\n";
    script << "export PYTHONPATH=\"" << abs_src_dir << ":" << abs_pkg_dir << ":${PYTHONPATH}\"\n";
    script << "exec python3 \"" << abs_main << "\" \"$@\"\n";
  }
  fs::permissions(launcher,
                  fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec |
                      fs::perms::owner_read | fs::perms::owner_write,
                  fs::perm_options::add);

  auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);
  std::cout << "Bundling [Python]: " << info.package_path << ":" << info.target_name << "\n";
  std::cout << "✓ Build successful: " << launcher << " (" << dur.count() << "ms)\n";
  return {};
}

// ---------------------------------------------------------------------------
// rust_binary builder
// ---------------------------------------------------------------------------

auto SimpleBuilder::build_rust_binary(const TargetInfo& info, const ParsedTarget& target)
    -> tl::expected<void, BuildError> {
  auto start = std::chrono::steady_clock::now();

  fs::path package_dir(info.package_path);
  fs::path output_dir = fs::path("bazel-bin") / info.package_path;
  fs::create_directories(output_dir);
  fs::path output_binary = output_dir / info.target_name;

  if (target.srcs.empty()) {
    std::cerr << "Error: No srcs for rust_binary " << info.target_name << "\n";
    return tl::unexpected(BuildError::SourceNotFound);
  }

  fs::path main_src = package_dir / target.srcs[0];
  if (!fs::exists(main_src)) {
    std::cerr << "Error: Source file not found: " << main_src << "\n";
    return tl::unexpected(BuildError::SourceNotFound);
  }

  // Incremental check
  if (fs::exists(output_binary) && !source_changed(main_src, output_binary)) {
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    std::cout << "✓ Target up-to-date: " << output_binary << " (" << dur.count()
              << "ms, incremental)\n";
    return {};
  }

  std::string edition = target.edition.empty() ? "2021" : target.edition;

  // rustc automatically resolves `mod greet;` to greet.rs in the same directory
  std::string cmd = "rustc --edition " + edition + " " + main_src.string() + " -o " +
                    output_binary.string() + " 2>&1";

  std::cout << "Compiling [Rust]: " << info.package_path << ":" << info.target_name << "\n";
  std::cout << "  " << main_src.string() << " (edition " << edition << ")\n";

  if (std::system(cmd.c_str()) != 0) {
    return tl::unexpected(BuildError::CompilationFailed);
  }

  auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);
  std::cout << "✓ Build successful: " << output_binary << " (" << dur.count() << "ms)\n";
  return {};
}

// ---------------------------------------------------------------------------
// Main build dispatch
// ---------------------------------------------------------------------------

auto SimpleBuilder::build(std::string_view target) -> tl::expected<void, BuildError> {
  std::cout << "Horcrux Build System\n";
  std::cout << "====================\n\n";

  auto target_info = parse_target(target);
  if (!target_info) {
    std::cerr << "Error: Invalid target format '" << target << "'\n";
    std::cerr << "Expected: //package/path:target_name\n";
    return tl::unexpected(target_info.error());
  }

  std::cout << "Target:  " << target << "\n";
  std::cout << "Package: " << target_info->package_path << "\n\n";

  if (!build_file_exists(target_info->package_path)) {
    std::cerr << "Error: BUILD file not found in " << target_info->package_path << "\n";
    return tl::unexpected(BuildError::BuildFileNotFound);
  }

  fs::path build_file = fs::path(target_info->package_path) / "BUILD";
  auto all_targets = parse_all_targets(build_file);

  if (all_targets.empty()) {
    std::cerr << "Error: No targets found in " << build_file << "\n";
    return tl::unexpected(BuildError::BuildFileNotFound);
  }

  // Find the requested target
  auto it = std::find_if(all_targets.begin(), all_targets.end(),
                         [&](const ParsedTarget& t) { return t.name == target_info->target_name; });

  if (it == all_targets.end()) {
    std::cerr << "Error: Target '" << target_info->target_name << "' not found in BUILD file\n";
    std::cerr << "Available targets:\n";
    for (const auto& t : all_targets) {
      std::cerr << "  //" << target_info->package_path << ":" << t.name << " (" << t.rule_type
                << ")\n";
    }
    return tl::unexpected(BuildError::InvalidTarget);
  }

  const auto& parsed = *it;

  std::cout << "Building target: " << target << "\n";

  if (parsed.rule_type == "cc_binary") {
    return compile_cc_binary(*target_info, parsed);
  }
  if (parsed.rule_type == "java_binary") {
    return build_java_binary(*target_info, parsed, all_targets);
  }
  if (parsed.rule_type == "py_binary") {
    return build_py_binary(*target_info, parsed);
  }
  if (parsed.rule_type == "rust_binary") {
    return build_rust_binary(*target_info, parsed);
  }

  std::cerr << "Error: Rule type '" << parsed.rule_type
            << "' is not yet supported for direct building.\n";
  std::cerr << "Supported rule types: cc_binary, java_binary, py_binary, rust_binary\n";
  return tl::unexpected(BuildError::InvalidTarget);
}

// ---------------------------------------------------------------------------
// Clean
// ---------------------------------------------------------------------------

auto SimpleBuilder::clean() -> tl::expected<void, BuildError> {
  std::cout << "Cleaning build artifacts...\n";
  fs::path output_dir = "bazel-bin";
  if (fs::exists(output_dir)) {
    std::error_code ec;
    fs::remove_all(output_dir, ec);
    if (ec) {
      std::cerr << "Warning: Could not fully clean " << output_dir << ": " << ec.message() << "\n";
    } else {
      std::cout << "✓ Removed: " << output_dir << "\n";
    }
  }
  std::cout << "Clean complete.\n";
  return {};
}

} // namespace horcrux::core
