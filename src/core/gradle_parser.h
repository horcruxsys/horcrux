// Horcrux - Gradle Parser
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <tl/expected.hpp>

namespace horcrux::core {

enum class GradleParserError {
  FileNotFound,
  InvalidSyntax,
  UnsupportedFormat,
  ParseError,
};

auto to_string(GradleParserError error) -> std::string;

struct GradleProject {
  std::string name;
  std::filesystem::path root_dir;
  std::vector<std::string> subprojects;
  std::map<std::string, std::string> properties;
};

struct GradleDependency {
  std::string group;
  std::string name;
  std::string version;
  std::string configuration; // e.g., "implementation", "testImplementation"
};

struct GradleSourceSet {
  std::string name;
  std::vector<std::filesystem::path> java_dirs;
  std::vector<std::filesystem::path> kotlin_dirs;
  std::vector<std::filesystem::path> resources_dirs;
  std::vector<std::filesystem::path> aidl_dirs;
  std::vector<std::filesystem::path> renderscript_dirs;
  std::vector<std::filesystem::path> jni_dirs;
};

struct GradleBuildVariant {
  std::string name;
  std::string build_type; // "debug" or "release"
  std::vector<std::string> flavors;
  std::map<std::string, std::string> config;
};

struct GradleBuildConfig {
  std::string project_name;
  std::string project_type; // "application", "library", "android-app", "android-library"
  std::string compile_sdk;
  std::string min_sdk;
  std::string target_sdk;
  std::string application_id;
  std::string version_name;
  std::string version_code;

  std::vector<GradleDependency> dependencies;
  std::vector<GradleSourceSet> source_sets;
  std::vector<GradleBuildVariant> build_variants;
  std::map<std::string, std::string> properties;
};

class GradleParser {
public:
  static auto parse_settings(const std::filesystem::path& settings_file)
      -> tl::expected<GradleProject, GradleParserError>;

  static auto parse_build(const std::filesystem::path& build_file)
      -> tl::expected<GradleBuildConfig, GradleParserError>;

  static auto detect_project_type(const std::filesystem::path& build_file)
      -> tl::expected<std::string, GradleParserError>;

private:
  static auto parse_groovy_file(const std::filesystem::path& file)
      -> tl::expected<std::string, GradleParserError>;

  static auto parse_kotlin_dsl_file(const std::filesystem::path& file)
      -> tl::expected<std::string, GradleParserError>;

  static auto extract_string_value(const std::string& content,
                                   const std::string& key) -> std::string;

  static auto extract_dependencies(const std::string& content) -> std::vector<GradleDependency>;

  static auto extract_source_sets(const std::string& content) -> std::vector<GradleSourceSet>;

  static auto extract_build_variants(const std::string& content) -> std::vector<GradleBuildVariant>;
};

} // namespace horcrux::core
