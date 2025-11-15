// Horcrux - Gradle Parser Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "gradle_parser.h"

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>

namespace horcrux::core {

auto to_string(GradleParserError error) -> std::string {
  switch (error) {
  case GradleParserError::FileNotFound:
    return "File not found";
  case GradleParserError::InvalidSyntax:
    return "Invalid syntax";
  case GradleParserError::UnsupportedFormat:
    return "Unsupported format";
  case GradleParserError::ParseError:
    return "Parse error";
  default:
    return "Unknown error";
  }
}

auto GradleParser::parse_groovy_file(const std::filesystem::path& file)
    -> tl::expected<std::string, GradleParserError> {
  std::ifstream input(file);
  if (!input.is_open()) {
    return tl::unexpected(GradleParserError::FileNotFound);
  }

  std::stringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

auto GradleParser::parse_kotlin_dsl_file(const std::filesystem::path& file)
    -> tl::expected<std::string, GradleParserError> {
  // Kotlin DSL files are parsed similarly to Groovy for basic extraction
  return parse_groovy_file(file);
}

auto GradleParser::extract_string_value(const std::string& content,
                                        const std::string& key) -> std::string {
  // Try multiple patterns in order of specificity

  // Pattern 1: Match quoted strings with = or :
  std::string pattern = key + R"(\s*[=:]\s*["']([^"']+)["'])";
  std::regex re(pattern);
  std::smatch match;

  if (std::regex_search(content, match, re) && match.size() > 1) {
    return match[1].str();
  }

  // Pattern 2: Match quoted strings in Kotlin DSL style (key = "value")
  pattern = key + R"(\s*=\s*\"([^\"]+)\")";
  re = std::regex(pattern);
  if (std::regex_search(content, match, re) && match.size() > 1) {
    return match[1].str();
  }

  // Pattern 3: Match numeric values with = (Kotlin DSL style: compileSdk = 34)
  pattern = key + R"(\s*=\s*(\d+))";
  re = std::regex(pattern);
  if (std::regex_search(content, match, re) && match.size() > 1) {
    return match[1].str();
  }

  // Pattern 4: Match unquoted numeric values with space (Groovy style: compileSdk 34)
  pattern = key + R"(\s+(\d+))";
  re = std::regex(pattern);
  if (std::regex_search(content, match, re) && match.size() > 1) {
    return match[1].str();
  }

  // Pattern 5: Match in defaultConfig block context
  pattern = key + R"(\s+\"([^\"]+)\")";
  re = std::regex(pattern);
  if (std::regex_search(content, match, re) && match.size() > 1) {
    return match[1].str();
  }

  return "";
}

auto GradleParser::extract_dependencies(const std::string& content)
    -> std::vector<GradleDependency> {
  std::vector<GradleDependency> dependencies;

  // Match dependency declarations like: implementation 'group:name:version'
  // or implementation("group:name:version")
  std::regex dep_re(
      R"((implementation|api|compile|testImplementation|androidTestImplementation)\s*[(\s]['"]([^'"]+)['"][\s)])");

  auto begin = std::sregex_iterator(content.begin(), content.end(), dep_re);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    std::smatch match = *it;
    std::string config = match[1].str();
    std::string dep_string = match[2].str();

    // Parse dependency string (format: group:name:version)
    std::regex parts_re(R"(([^:]+):([^:]+):([^:]+))");
    std::smatch parts_match;

    if (std::regex_search(dep_string, parts_match, parts_re)) {
      GradleDependency dep;
      dep.configuration = config;
      dep.group = parts_match[1].str();
      dep.name = parts_match[2].str();
      dep.version = parts_match[3].str();
      dependencies.push_back(dep);
    }
  }

  return dependencies;
}

auto GradleParser::extract_source_sets(const std::string& content) -> std::vector<GradleSourceSet> {
  std::vector<GradleSourceSet> source_sets;

  // Default Android source sets
  GradleSourceSet main_set;
  main_set.name = "main";
  main_set.java_dirs.push_back("src/main/java");
  main_set.kotlin_dirs.push_back("src/main/kotlin");
  main_set.resources_dirs.push_back("src/main/res");
  source_sets.push_back(main_set);

  GradleSourceSet test_set;
  test_set.name = "test";
  test_set.java_dirs.push_back("src/test/java");
  test_set.kotlin_dirs.push_back("src/test/kotlin");
  source_sets.push_back(test_set);

  return source_sets;
}

auto GradleParser::extract_build_variants(const std::string& content)
    -> std::vector<GradleBuildVariant> {
  std::vector<GradleBuildVariant> variants;

  // Default build types
  GradleBuildVariant debug_variant;
  debug_variant.name = "debug";
  debug_variant.build_type = "debug";
  variants.push_back(debug_variant);

  GradleBuildVariant release_variant;
  release_variant.name = "release";
  release_variant.build_type = "release";
  variants.push_back(release_variant);

  return variants;
}

auto GradleParser::parse_settings(const std::filesystem::path& settings_file)
    -> tl::expected<GradleProject, GradleParserError> {
  if (!std::filesystem::exists(settings_file)) {
    return tl::unexpected(GradleParserError::FileNotFound);
  }

  auto content_result = parse_groovy_file(settings_file);
  if (!content_result) {
    return tl::unexpected(content_result.error());
  }

  const auto& content = *content_result;
  GradleProject project;
  project.root_dir = settings_file.parent_path();

  // Extract project name
  project.name = extract_string_value(content, "rootProject.name");
  if (project.name.empty()) {
    project.name = project.root_dir.filename().string();
  }

  // Extract subprojects (include statements)
  std::regex include_re(R"(include\s*[(\s]['"]([^'"]+)['"])");
  auto begin = std::sregex_iterator(content.begin(), content.end(), include_re);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    std::smatch match = *it;
    project.subprojects.push_back(match[1].str());
  }

  return project;
}

auto GradleParser::parse_build(const std::filesystem::path& build_file)
    -> tl::expected<GradleBuildConfig, GradleParserError> {
  if (!std::filesystem::exists(build_file)) {
    return tl::unexpected(GradleParserError::FileNotFound);
  }

  auto content_result = (build_file.extension() == ".kts") ? parse_kotlin_dsl_file(build_file)
                                                           : parse_groovy_file(build_file);

  if (!content_result) {
    return tl::unexpected(content_result.error());
  }

  const auto& content = *content_result;
  GradleBuildConfig config;

  // Detect project type
  auto type_result = detect_project_type(build_file);
  if (type_result) {
    config.project_type = *type_result;
  }

  // Extract Android configuration
  config.compile_sdk = extract_string_value(content, "compileSdk");
  if (config.compile_sdk.empty()) {
    config.compile_sdk = extract_string_value(content, "compileSdkVersion");
  }

  config.min_sdk = extract_string_value(content, "minSdk");
  if (config.min_sdk.empty()) {
    config.min_sdk = extract_string_value(content, "minSdkVersion");
  }

  config.target_sdk = extract_string_value(content, "targetSdk");
  if (config.target_sdk.empty()) {
    config.target_sdk = extract_string_value(content, "targetSdkVersion");
  }

  config.application_id = extract_string_value(content, "applicationId");
  config.version_name = extract_string_value(content, "versionName");
  config.version_code = extract_string_value(content, "versionCode");

  // Extract dependencies
  config.dependencies = extract_dependencies(content);

  // Extract source sets
  config.source_sets = extract_source_sets(content);

  // Extract build variants
  config.build_variants = extract_build_variants(content);

  config.project_name = build_file.parent_path().filename().string();

  return config;
}

auto GradleParser::detect_project_type(const std::filesystem::path& build_file)
    -> tl::expected<std::string, GradleParserError> {
  if (!std::filesystem::exists(build_file)) {
    return tl::unexpected(GradleParserError::FileNotFound);
  }

  auto content_result = (build_file.extension() == ".kts") ? parse_kotlin_dsl_file(build_file)
                                                           : parse_groovy_file(build_file);

  if (!content_result) {
    return tl::unexpected(content_result.error());
  }

  const auto& content = *content_result;

  // Check for Android application
  if (content.find("com.android.application") != std::string::npos) {
    return "android-app";
  }

  // Check for Android library
  if (content.find("com.android.library") != std::string::npos) {
    return "android-library";
  }

  // Check for Java application
  if (content.find("application") != std::string::npos ||
      content.find("java-application") != std::string::npos) {
    return "application";
  }

  // Check for Java library
  if (content.find("java-library") != std::string::npos ||
      content.find("java") != std::string::npos) {
    return "library";
  }

  return "library"; // Default to library
}

} // namespace horcrux::core
