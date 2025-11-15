// Horcrux - Simple Builder Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "simple_builder.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>

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
  // Initialize build cache in .horcrux-cache directory
  auto cache_result = LocalCache::create(".horcrux-cache");
  if (cache_result) {
    build_cache_ = std::move(*cache_result);
  }
  // If cache creation fails, we'll just proceed without caching
}

auto SimpleBuilder::parse_target(std::string_view target) -> tl::expected<TargetInfo, BuildError> {
  // Target format: //package/path:target_name
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
  fs::path build_file_path = fs::path(package_path) / "BUILD";
  return fs::exists(build_file_path);
}

auto SimpleBuilder::get_compiler() -> const std::string& {
  if (!cached_compiler_) {
    // Cache the compiler path on first call
    if (std::system("which g++ > /dev/null 2>&1") == 0) {
      cached_compiler_ = "g++";
    } else if (std::system("which clang++ > /dev/null 2>&1") == 0) {
      cached_compiler_ = "clang++";
    } else {
      cached_compiler_ = "c++"; // Fallback
    }
  }
  return *cached_compiler_;
}

auto SimpleBuilder::source_changed(const fs::path& source_file,
                                    const fs::path& output_binary) -> bool {
  // If output doesn't exist, source has "changed"
  if (!fs::exists(output_binary)) {
    return true;
  }

  // Check file modification times
  auto source_time = fs::last_write_time(source_file);
  auto output_time = fs::last_write_time(output_binary);

  return source_time > output_time;
}

auto SimpleBuilder::compile_cc_binary(const TargetInfo& target_info)
    -> tl::expected<void, BuildError> {
  auto start_time = std::chrono::steady_clock::now();

  // Get source directory
  fs::path source_dir = fs::path(target_info.package_path);

  // Check if BUILD file exists
  if (!build_file_exists(target_info.package_path)) {
    std::cerr << "Error: BUILD file not found in " << target_info.package_path << "\n";
    return tl::unexpected(BuildError::BuildFileNotFound);
  }

  // For this bootstrap implementation, we assume main.cpp exists
  fs::path source_file = source_dir / "main.cpp";
  if (!fs::exists(source_file)) {
    std::cerr << "Error: Source file not found: " << source_file << "\n";
    return tl::unexpected(BuildError::SourceNotFound);
  }

  // Create output directory structure
  fs::path output_dir = fs::path("bazel-bin") / target_info.package_path;
  fs::create_directories(output_dir);

  fs::path output_binary = output_dir / target_info.target_name;

  std::cout << "Building target: //" << target_info.package_path << ":" << target_info.target_name
            << "\n";

  // Check if we can use cached result (incremental build)
  if (!source_changed(source_file, output_binary)) {
    std::cout << "✓ Target up-to-date (cached): " << output_binary << "\n";
    auto end_time = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Build time: " << duration.count() << "ms (incremental)\n";
    return {};
  }

  // Check cache for compiled binary
  if (build_cache_) {
    // Read source file content to compute hash
    std::ifstream file(source_file, std::ios::binary);
    if (file) {
      std::vector<uint8_t> source_content((std::istreambuf_iterator<char>(file)),
                                          std::istreambuf_iterator<char>());

      // Compute hash of source file
      auto source_hash = compute_sha256(source_content);

      // Check if cached binary exists
      auto cached_artifact = build_cache_->lookup(source_hash);
      if (cached_artifact) {
        std::cout << "✓ Found cached binary (hash: " << hash_to_string(source_hash).substr(0, 8)
                  << "...)\n";

        // Write cached binary to output
        std::ofstream output(output_binary, std::ios::binary);
        if (output) {
          output.write(reinterpret_cast<const char*>(cached_artifact->content.data()),
                       static_cast<std::streamsize>(cached_artifact->content.size()));

          // Make executable
          fs::permissions(output_binary, fs::perms::owner_exec | fs::perms::group_exec |
                                             fs::perms::others_exec,
                          fs::perm_options::add);

          auto end_time = std::chrono::steady_clock::now();
          auto duration =
              std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
          std::cout << "✓ Build successful (from cache): " << output_binary << "\n";
          std::cout << "Build time: " << duration.count() << "ms (cached)\n";
          return {};
        }
      }
    }
  }

  // Use cached compiler path
  const auto& compiler = get_compiler();

  // Build compilation command
  std::string compile_cmd = compiler + " -std=c++23 -O2 -Wall -Wextra " + source_file.string() +
                            " -o " + output_binary.string() + " 2>&1";

  std::cout << "Compiling: " << source_file << "\n";
  std::cout << "Output: " << output_binary << "\n";

  // Execute compilation
  int result = std::system(compile_cmd.c_str());

  if (result != 0) {
    std::cerr << "Error: Compilation failed with exit code " << result << "\n";
    return tl::unexpected(BuildError::CompilationFailed);
  }

  // Cache the compiled binary
  if (build_cache_) {
    std::ifstream source_file_stream(source_file, std::ios::binary);
    std::ifstream binary_file(output_binary, std::ios::binary);

    if (source_file_stream && binary_file) {
      std::vector<uint8_t> source_content((std::istreambuf_iterator<char>(source_file_stream)),
                                          std::istreambuf_iterator<char>());
      std::vector<uint8_t> binary_content((std::istreambuf_iterator<char>(binary_file)),
                                          std::istreambuf_iterator<char>());

      auto source_hash = compute_sha256(source_content);

      Artifact artifact;
      artifact.content = std::move(binary_content);
      artifact.timestamp =
          std::chrono::system_clock::now().time_since_epoch().count();

      auto store_result = build_cache_->store(source_hash, artifact);
      if (store_result) {
        std::cout << "✓ Cached binary (hash: " << hash_to_string(source_hash).substr(0, 8)
                  << "...)\n";
      }
    }
  }

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  std::cout << "✓ Build successful: " << output_binary << "\n";
  std::cout << "Build time: " << duration.count() << "ms (full build)\n";

  return {};
}

auto SimpleBuilder::build(std::string_view target) -> tl::expected<void, BuildError> {
  std::cout << "Horcrux Build System (Bootstrap)\n";
  std::cout << "=================================\n\n";

  // Parse target
  auto target_info = parse_target(target);
  if (!target_info) {
    std::cerr << "Error: Invalid target format '" << target << "'\n";
    std::cerr << "Expected format: //package/path:target_name\n";
    return tl::unexpected(target_info.error());
  }

  std::cout << "Target: " << target << "\n";
  std::cout << "Package: " << target_info->package_path << "\n";
  std::cout << "Name: " << target_info->target_name << "\n\n";

  // For this bootstrap, we only support cc_binary
  // A full implementation would parse the BUILD file and handle different rule types
  return compile_cc_binary(*target_info);
}

auto SimpleBuilder::clean() -> tl::expected<void, BuildError> {
  std::cout << "Cleaning build artifacts...\n";

  // Remove bazel-bin directory
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
