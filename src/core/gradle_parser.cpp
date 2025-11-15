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

  // Extract flavor dimensions (comma-separated or quoted strings)
  std::vector<std::string> dimensions;

  // Look for flavorDimensions line
  size_t dim_start = content.find("flavorDimensions");
  if (dim_start != std::string::npos) {
    // Find end of line or closing brace
    size_t dim_end = content.find('\n', dim_start);
    if (dim_end == std::string::npos) {
      dim_end = content.find('}', dim_start);
    }

    if (dim_end != std::string::npos) {
      std::string dim_line = content.substr(dim_start, dim_end - dim_start);

      // Extract all quoted strings from the line
      std::regex quoted_re(R"(["']([^"']+)["'])");
      auto quote_begin = std::sregex_iterator(dim_line.begin(), dim_line.end(), quoted_re);
      auto quote_end = std::sregex_iterator();

      for (auto it = quote_begin; it != quote_end; ++it) {
        std::string dim = (*it)[1].str();
        // Trim whitespace
        size_t start = dim.find_first_not_of(" \t\n\r");
        size_t end = dim.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos) {
          dimensions.push_back(dim.substr(start, end - start + 1));
        }
      }
    }
  }

  // Extract product flavors - look for flavor names as identifiers before opening brace
  std::map<std::string, std::vector<std::string>> flavor_by_dimension;

  // Find productFlavors block
  size_t flavors_start = content.find("productFlavors");
  if (flavors_start != std::string::npos) {
    // Find the opening brace for productFlavors
    size_t block_start = content.find('{', flavors_start);
    if (block_start != std::string::npos) {
      // Find matching closing brace (simple approach - count braces)
      int brace_count = 1;
      size_t block_end = block_start + 1;
      while (block_end < content.size() && brace_count > 0) {
        if (content[block_end] == '{')
          brace_count++;
        if (content[block_end] == '}')
          brace_count--;
        block_end++;
      }

      std::string flavors_block = content.substr(block_start + 1, block_end - block_start - 2);

      // Extract flavor names - look for identifier followed by {
      std::regex flavor_re(R"(\b(\w+)\s*\{)");
      auto flavor_begin =
          std::sregex_iterator(flavors_block.begin(), flavors_block.end(), flavor_re);
      auto flavor_end = std::sregex_iterator();

      for (auto it = flavor_begin; it != flavor_end; ++it) {
        std::string flavor_name = (*it)[1].str();

        // Find dimension for this flavor
        std::string dimension;
        std::regex dim_re2(R"(dimension\s+["']([^"']+)["'])");
        std::smatch dim_match2;

        // Search for dimension in the flavor's block
        size_t flavor_pos = flavors_block.find(flavor_name);
        if (flavor_pos != std::string::npos) {
          size_t flavor_block_start = flavors_block.find('{', flavor_pos);
          if (flavor_block_start != std::string::npos) {
            // Find matching brace
            int count = 1;
            size_t flavor_block_end = flavor_block_start + 1;
            while (flavor_block_end < flavors_block.size() && count > 0) {
              if (flavors_block[flavor_block_end] == '{')
                count++;
              if (flavors_block[flavor_block_end] == '}')
                count--;
              flavor_block_end++;
            }
            std::string flavor_content =
                flavors_block.substr(flavor_block_start, flavor_block_end - flavor_block_start);

            if (std::regex_search(flavor_content, dim_match2, dim_re2)) {
              dimension = dim_match2[1].str();
            }
          }
        }

        // If no dimension found, use first dimension if available
        if (dimension.empty() && !dimensions.empty()) {
          dimension = dimensions[0];
        }

        if (!dimension.empty()) {
          flavor_by_dimension[dimension].push_back(flavor_name);
        }
      }
    }
  }

  // Extract build types - similar approach
  std::vector<std::string> build_types;

  size_t build_types_start = content.find("buildTypes");
  if (build_types_start != std::string::npos) {
    size_t block_start = content.find('{', build_types_start);
    if (block_start != std::string::npos) {
      int brace_count = 1;
      size_t block_end = block_start + 1;
      while (block_end < content.size() && brace_count > 0) {
        if (content[block_end] == '{')
          brace_count++;
        if (content[block_end] == '}')
          brace_count--;
        block_end++;
      }

      std::string build_types_block = content.substr(block_start + 1, block_end - block_start - 2);

      std::regex bt_re(R"(\b(\w+)\s*\{)");
      auto bt_begin =
          std::sregex_iterator(build_types_block.begin(), build_types_block.end(), bt_re);
      auto bt_end = std::sregex_iterator();

      for (auto it = bt_begin; it != bt_end; ++it) {
        build_types.push_back((*it)[1].str());
      }
    }
  }

  // If no build types found, use defaults
  if (build_types.empty()) {
    build_types.push_back("debug");
    build_types.push_back("release");
  }

  // Generate all variant combinations
  if (flavor_by_dimension.empty()) {
    // No flavors, just build types
    for (const auto& build_type : build_types) {
      GradleBuildVariant variant;
      variant.name = build_type;
      variant.build_type = build_type;
      variants.push_back(variant);
    }
  } else {
    // Generate cartesian product of flavors across dimensions
    std::function<void(size_t, std::vector<std::string>&)> generate_flavor_combos;

    std::vector<std::vector<std::string>> flavor_combos;

    generate_flavor_combos = [&](size_t dim_idx, std::vector<std::string>& current) {
      if (dim_idx >= dimensions.size()) {
        flavor_combos.push_back(current);
        return;
      }

      const auto& current_dim = dimensions[dim_idx];
      if (flavor_by_dimension.find(current_dim) != flavor_by_dimension.end()) {
        for (const auto& flavor : flavor_by_dimension[current_dim]) {
          current.push_back(flavor);
          generate_flavor_combos(dim_idx + 1, current);
          current.pop_back();
        }
      } else {
        // Skip this dimension if no flavors found
        generate_flavor_combos(dim_idx + 1, current);
      }
    };

    std::vector<std::string> current_combo;
    generate_flavor_combos(0, current_combo);

    // Generate variant for each build type and flavor combination
    for (const auto& build_type : build_types) {
      for (const auto& flavor_combo : flavor_combos) {
        GradleBuildVariant variant;
        variant.flavors = flavor_combo;
        variant.build_type = build_type;

        // Generate variant name (e.g., freeDebug, proRelease)
        std::string name;
        for (size_t i = 0; i < flavor_combo.size(); ++i) {
          std::string flavor_name = flavor_combo[i];
          if (i > 0 && !flavor_name.empty()) {
            flavor_name[0] =
                static_cast<char>(std::toupper(static_cast<unsigned char>(flavor_name[0])));
          }
          name += flavor_name;
        }

        // Capitalize build type
        std::string bt_name = build_type;
        if (!bt_name.empty()) {
          bt_name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(bt_name[0])));
        }
        name += bt_name;

        variant.name = name;
        variants.push_back(variant);
      }
    }
  }

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
