// Horcrux - Simple Logging System
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <iostream>
#include <string_view>

namespace horcrux::cli {

/// @brief Log levels for the build system
enum class LogLevel { Debug, Info, Warning, Error };

/// @brief Simple logger for CLI output
class Logger {
public:
  explicit Logger(LogLevel level = LogLevel::Info) : level_(level) {
  }

  /// @brief Log a debug message
  template <typename... Args>
  void debug(Args&&... args) {
    if (level_ <= LogLevel::Debug) {
      std::cout << "[DEBUG] ";
      (std::cout << ... << args) << '\n';
    }
  }

  /// @brief Log an info message
  template <typename... Args>
  void info(Args&&... args) {
    if (level_ <= LogLevel::Info) {
      std::cout << "[INFO] ";
      (std::cout << ... << args) << '\n';
    }
  }

  /// @brief Log a warning message
  template <typename... Args>
  void warning(Args&&... args) {
    if (level_ <= LogLevel::Warning) {
      std::cout << "[WARNING] ";
      (std::cout << ... << args) << '\n';
    }
  }

  /// @brief Log an error message
  template <typename... Args>
  void error(Args&&... args) {
    if (level_ <= LogLevel::Error) {
      std::cerr << "[ERROR] ";
      (std::cerr << ... << args) << '\n';
    }
  }

  /// @brief Set the log level
  void set_level(LogLevel level) {
    level_ = level;
  }

  /// @brief Get the current log level
  [[nodiscard]] auto level() const -> LogLevel {
    return level_;
  }

private:
  LogLevel level_;
};

/// @brief Global logger instance
inline Logger global_logger{LogLevel::Info};

} // namespace horcrux::cli
