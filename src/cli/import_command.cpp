// Horcrux - Import Command Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "import_command.h"
#include "logger.h"

#include "../core/gradle_parser.h"
#include "../core/workspace_config_generator.h"

#include <iostream>

namespace horcrux::cli {

auto to_string(ImportError error) -> std::string {
  switch (error) {
  case ImportError::InvalidPath:
    return "Invalid project path";
  case ImportError::NoGradleFiles:
    return "No Gradle files found in project";
  case ImportError::ParseError:
    return "Failed to parse Gradle files";
  case ImportError::GenerateError:
    return "Failed to generate configuration";
  case ImportError::FileSystemError:
    return "File system error";
  default:
    return "Unknown error";
  }
}

ImportCommand::ImportCommand(Logger& logger) : logger_(logger) {}

auto ImportCommand::validate_project_path(const std::filesystem::path& path)
    -> std::expected<void, ImportError> {
  if (!std::filesystem::exists(path)) {
    return std::unexpected(ImportError::InvalidPath);
  }

  if (!std::filesystem::is_directory(path)) {
    return std::unexpected(ImportError::InvalidPath);
  }

  return {};
}

auto ImportCommand::find_gradle_files(const std::filesystem::path& project_path)
    -> std::expected<std::pair<std::filesystem::path, std::filesystem::path>, ImportError> {
  
  std::filesystem::path settings_file;
  std::filesystem::path build_file;

  // Check for settings.gradle or settings.gradle.kts
  auto settings_groovy = project_path / "settings.gradle";
  auto settings_kts = project_path / "settings.gradle.kts";
  
  if (std::filesystem::exists(settings_groovy)) {
    settings_file = settings_groovy;
  } else if (std::filesystem::exists(settings_kts)) {
    settings_file = settings_kts;
  }

  // Check for build.gradle or build.gradle.kts
  auto build_groovy = project_path / "build.gradle";
  auto build_kts = project_path / "build.gradle.kts";
  
  if (std::filesystem::exists(build_groovy)) {
    build_file = build_groovy;
  } else if (std::filesystem::exists(build_kts)) {
    build_file = build_kts;
  }

  // If we have at least a build file, we can proceed
  if (build_file.empty()) {
    return std::unexpected(ImportError::NoGradleFiles);
  }

  return std::make_pair(settings_file, build_file);
}

auto ImportCommand::execute(const std::filesystem::path& project_path) 
    -> std::expected<void, ImportError> {
  auto output_path = project_path / "horcrux.yaml";
  return execute_with_options(project_path, output_path);
}

auto ImportCommand::execute_with_options(
    const std::filesystem::path& project_path,
    const std::filesystem::path& output_path)
    -> std::expected<void, ImportError> {
  
  // Validate project path
  auto validation = validate_project_path(project_path);
  if (!validation) {
    return std::unexpected(validation.error());
  }

  logger_.info("Importing Gradle project from: ", project_path.string());

  // Find Gradle files
  auto files_result = find_gradle_files(project_path);
  if (!files_result) {
    return std::unexpected(files_result.error());
  }

  auto [settings_file, build_file] = *files_result;

  // Parse settings.gradle if it exists
  core::GradleProject project;
  if (!settings_file.empty()) {
    logger_.info("Parsing settings file: ", settings_file.filename().string());
    auto project_result = core::GradleParser::parse_settings(settings_file);
    if (!project_result) {
      logger_.error("Failed to parse settings.gradle: ", 
                   core::to_string(project_result.error()));
      return std::unexpected(ImportError::ParseError);
    }
    project = *project_result;
    logger_.info("Project name: ", project.name);
  } else {
    // Create default project
    project.name = project_path.filename().string();
    project.root_dir = project_path;
    logger_.info("No settings.gradle found, using directory name: ", project.name);
  }

  // Parse build.gradle
  logger_.info("Parsing build file: ", build_file.filename().string());
  auto build_config_result = core::GradleParser::parse_build(build_file);
  if (!build_config_result) {
    logger_.error("Failed to parse build.gradle: ", 
                 core::to_string(build_config_result.error()));
    return std::unexpected(ImportError::ParseError);
  }

  auto build_config = *build_config_result;
  logger_.info("Project type: ", build_config.project_type);
  logger_.info("Found ", build_config.dependencies.size(), " dependencies");
  logger_.info("Found ", build_config.source_sets.size(), " source sets");

  // Generate horcrux.yaml
  logger_.info("Generating horcrux.yaml...");
  auto generate_result = core::WorkspaceConfigGenerator::generate_from_gradle(
      project, build_config, output_path);
  
  if (!generate_result) {
    logger_.error("Failed to generate horcrux.yaml: ", 
                 core::to_string(generate_result.error()));
    return std::unexpected(ImportError::GenerateError);
  }

  logger_.info("Successfully generated: ", output_path.string());
  return {};
}

auto handle_import_command(int argc, char* argv[], Logger& logger) -> int {
  if (argc < 3) {
    logger.error("Missing project path");
    std::cerr << "Usage: horcrux import <project-path>\n";
    std::cerr << "Example: horcrux import .\n";
    std::cerr << "Example: horcrux import /path/to/android/project\n";
    return 1;
  }

  std::filesystem::path project_path = argv[2];

  // Check for custom output path
  std::filesystem::path output_path = project_path / "horcrux.yaml";
  for (int i = 3; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg.starts_with("--output=")) {
      output_path = arg.substr(9);
    } else if (arg == "-o" && i + 1 < argc) {
      output_path = argv[++i];
    }
  }

  ImportCommand command(logger);
  auto result = command.execute_with_options(project_path, output_path);
  
  if (!result) {
    logger.error("Import failed: ", to_string(result.error()));
    return 1;
  }

  return 0;
}

} // namespace horcrux::cli
