// Horcrux - Workspace Config Generator Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "workspace_config_generator.h"

#include <fstream>
#include <sstream>

namespace horcrux::core {

auto to_string(ConfigGeneratorError error) -> std::string {
  switch (error) {
  case ConfigGeneratorError::FileWriteError:
    return "Failed to write file";
  case ConfigGeneratorError::InvalidConfig:
    return "Invalid configuration";
  case ConfigGeneratorError::DirectoryError:
    return "Directory error";
  default:
    return "Unknown error";
  }
}

auto WorkspaceConfigGenerator::map_gradle_dependency_to_yaml(const GradleDependency& dep)
    -> std::string {
  std::stringstream ss;
  ss << "  - name: " << dep.name << "\n";
  ss << "    group: " << dep.group << "\n";
  ss << "    version: " << dep.version << "\n";
  ss << "    scope: " << dep.configuration << "\n";
  return ss.str();
}

auto WorkspaceConfigGenerator::map_source_set_to_yaml(const GradleSourceSet& source_set)
    -> std::string {
  std::stringstream ss;
  ss << "  " << source_set.name << ":\n";

  if (!source_set.java_dirs.empty()) {
    ss << "    java:\n";
    for (const auto& dir : source_set.java_dirs) {
      ss << "      - " << dir.string() << "\n";
    }
  }

  if (!source_set.kotlin_dirs.empty()) {
    ss << "    kotlin:\n";
    for (const auto& dir : source_set.kotlin_dirs) {
      ss << "      - " << dir.string() << "\n";
    }
  }

  if (!source_set.resources_dirs.empty()) {
    ss << "    resources:\n";
    for (const auto& dir : source_set.resources_dirs) {
      ss << "      - " << dir.string() << "\n";
    }
  }

  return ss.str();
}

auto WorkspaceConfigGenerator::map_build_variant_to_yaml(const GradleBuildVariant& variant)
    -> std::string {
  std::stringstream ss;
  ss << "  " << variant.name << ":\n";
  ss << "    type: " << variant.build_type << "\n";

  if (!variant.flavors.empty()) {
    ss << "    flavors:\n";
    for (const auto& flavor : variant.flavors) {
      ss << "      - " << flavor << "\n";
    }
  }

  if (!variant.config.empty()) {
    ss << "    config:\n";
    for (const auto& [key, value] : variant.config) {
      ss << "      " << key << ": " << value << "\n";
    }
  }

  return ss.str();
}

auto WorkspaceConfigGenerator::generate_yaml_content(
    const GradleProject& project, const GradleBuildConfig& build_config) -> std::string {
  std::stringstream yaml;

  // Header
  yaml << "# Horcrux Workspace Configuration\n";
  yaml << "# Generated from Gradle project\n";
  yaml << "# Project: " << project.name << "\n\n";

  // Workspace metadata
  yaml << "workspace:\n";
  yaml << "  name: " << project.name << "\n";
  yaml << "  version: \"0.1.0\"\n";
  yaml << "  type: " << build_config.project_type << "\n\n";

  // Android configuration (if applicable)
  if (build_config.project_type.find("android") != std::string::npos) {
    yaml << "android:\n";
    if (!build_config.compile_sdk.empty()) {
      yaml << "  compileSdk: " << build_config.compile_sdk << "\n";
    }
    if (!build_config.min_sdk.empty()) {
      yaml << "  minSdk: " << build_config.min_sdk << "\n";
    }
    if (!build_config.target_sdk.empty()) {
      yaml << "  targetSdk: " << build_config.target_sdk << "\n";
    }
    if (!build_config.application_id.empty()) {
      yaml << "  applicationId: " << build_config.application_id << "\n";
    }
    if (!build_config.version_name.empty()) {
      yaml << "  versionName: " << build_config.version_name << "\n";
    }
    if (!build_config.version_code.empty()) {
      yaml << "  versionCode: " << build_config.version_code << "\n";
    }
    yaml << "\n";
  }

  // Source sets
  if (!build_config.source_sets.empty()) {
    yaml << "sourceSets:\n";
    for (const auto& source_set : build_config.source_sets) {
      yaml << map_source_set_to_yaml(source_set);
    }
    yaml << "\n";
  }

  // Build variants
  if (!build_config.build_variants.empty()) {
    yaml << "buildVariants:\n";
    for (const auto& variant : build_config.build_variants) {
      yaml << map_build_variant_to_yaml(variant);
    }
    yaml << "\n";
  }

  // Dependencies
  if (!build_config.dependencies.empty()) {
    yaml << "dependencies:\n";
    for (const auto& dep : build_config.dependencies) {
      yaml << map_gradle_dependency_to_yaml(dep);
    }
    yaml << "\n";
  }

  // Build targets
  yaml << "targets:\n";
  yaml << "  app:\n";
  yaml << "    rule: ";
  if (build_config.project_type == "android-app") {
    yaml << "android_app\n";
  } else if (build_config.project_type == "android-library") {
    yaml << "android_library\n";
  } else if (build_config.project_type == "application") {
    yaml << "java_application\n";
  } else {
    yaml << "java_library\n";
  }
  yaml << "    srcs:\n";
  for (const auto& source_set : build_config.source_sets) {
    if (source_set.name == "main") {
      for (const auto& dir : source_set.java_dirs) {
        yaml << "      - " << dir.string() << "/**/*.java\n";
      }
      for (const auto& dir : source_set.kotlin_dirs) {
        yaml << "      - " << dir.string() << "/**/*.kt\n";
      }
    }
  }
  yaml << "    deps:\n";
  for (const auto& dep : build_config.dependencies) {
    if (dep.configuration == "implementation" || dep.configuration == "api") {
      yaml << "      - \"" << dep.group << ":" << dep.name << ":" << dep.version << "\"\n";
    }
  }

  return yaml.str();
}

auto WorkspaceConfigGenerator::generate_from_gradle(
    const GradleProject& project, const GradleBuildConfig& build_config,
    const std::filesystem::path& output_path) -> tl::expected<void, ConfigGeneratorError> {
  // Generate YAML content
  std::string yaml_content = generate_yaml_content(project, build_config);

  // Write to file
  std::ofstream output(output_path);
  if (!output.is_open()) {
    return tl::unexpected(ConfigGeneratorError::FileWriteError);
  }

  output << yaml_content;
  output.close();

  return {};
}

} // namespace horcrux::core
