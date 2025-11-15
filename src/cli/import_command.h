// Horcrux - Import Command
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace horcrux::cli {

// Forward declarations
class Logger;

enum class ImportError {
  InvalidPath,
  NoGradleFiles,
  ParseError,
  GenerateError,
  FileSystemError,
};

auto to_string(ImportError error) -> std::string;

class ImportCommand {
public:
  explicit ImportCommand(Logger& logger);

  auto execute(const std::filesystem::path& project_path) 
      -> std::expected<void, ImportError>;

  auto execute_with_options(
      const std::filesystem::path& project_path,
      const std::filesystem::path& output_path)
      -> std::expected<void, ImportError>;

private:
  Logger& logger_;

  auto find_gradle_files(const std::filesystem::path& project_path)
      -> std::expected<std::pair<std::filesystem::path, std::filesystem::path>, ImportError>;

  auto validate_project_path(const std::filesystem::path& path)
      -> std::expected<void, ImportError>;
};

auto handle_import_command(int argc, char* argv[], Logger& logger) -> int;

} // namespace horcrux::cli
