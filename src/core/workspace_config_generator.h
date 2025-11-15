// Horcrux - Workspace Config Generator
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <string>

#include <tl/expected.hpp>

#include "gradle_parser.h"

namespace horcrux::core {

enum class ConfigGeneratorError {
  FileWriteError,
  InvalidConfig,
  DirectoryError,
};

auto to_string(ConfigGeneratorError error) -> std::string;

class WorkspaceConfigGenerator {
public:
  static auto generate_from_gradle(
      const GradleProject& project, const GradleBuildConfig& build_config,
      const std::filesystem::path& output_path) -> tl::expected<void, ConfigGeneratorError>;

private:
  static auto generate_yaml_content(const GradleProject& project,
                                    const GradleBuildConfig& build_config) -> std::string;

  static auto map_gradle_dependency_to_yaml(const GradleDependency& dep) -> std::string;

  static auto map_source_set_to_yaml(const GradleSourceSet& source_set) -> std::string;

  static auto map_build_variant_to_yaml(const GradleBuildVariant& variant) -> std::string;
};

} // namespace horcrux::core
