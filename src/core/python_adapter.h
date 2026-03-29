// Horcrux - Python Adapter
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <string>
#include <vector>

#include "adapter.h"

namespace horcrux::core {

/// @brief Toolchain information for Python execution
struct PythonToolchain {
  std::string python;         ///< Path to python interpreter (e.g., "python3")
  std::string python_version; ///< Python version string (e.g., "3.11.0")
};

/// @brief Python language adapter supporting py_library, py_binary, py_test
///
/// Implements source packaging and test execution with deterministic cache keys.
/// Toolchain detection falls back: HORCRUX_PYTHON env var → python3 → python on PATH.
class PythonAdapter final : public Adapter {
public:
  /// @brief Create a PythonAdapter, detecting the Python toolchain
  /// @return PythonAdapter or error if no suitable toolchain found
  static auto create() -> tl::expected<PythonAdapter, AdapterError>;

  /// @brief Create a PythonAdapter with an explicit toolchain (for testing)
  explicit PythonAdapter(PythonToolchain toolchain);

  ~PythonAdapter() override = default;
  PythonAdapter(PythonAdapter&&) = default;
  PythonAdapter& operator=(PythonAdapter&&) = default;

  [[nodiscard]] auto info() const -> const AdapterInfo& override;

  [[nodiscard]] auto
  parse_target(const BuildNode& node) -> tl::expected<AdapterTarget, AdapterError> override;

  [[nodiscard]] auto plan_actions(const AdapterTarget& target, const BuildGraph& graph,
                                  const std::string& output_root)
      -> tl::expected<std::vector<BuildAction>, AdapterError> override;

  [[nodiscard]] auto
  compute_cache_key(const AdapterTarget& target) -> tl::expected<Hash, AdapterError> override;

  [[nodiscard]] auto diagnostics() const -> const std::vector<Diagnostic>& override;

  /// @brief Get the detected toolchain
  [[nodiscard]] auto toolchain() const -> const PythonToolchain& {
    return toolchain_;
  }

private:
  AdapterInfo info_;
  PythonToolchain toolchain_;
  mutable std::vector<Diagnostic> diagnostics_;

  void emit(Diagnostic::Level level, std::string message,
            std::optional<std::string> location = std::nullopt) const;

  auto plan_package_action(const AdapterTarget& target, const std::string& output_root) const
      -> tl::expected<BuildAction, AdapterError>;

  auto plan_test_action(const AdapterTarget& target, const std::string& output_root) const
      -> tl::expected<BuildAction, AdapterError>;

  static auto detect_toolchain() -> tl::expected<PythonToolchain, AdapterError>;
  static auto label_target_name(const std::string& label) -> std::string;
  static auto label_to_safe_path(const std::string& label) -> std::string;
  static auto split_tokens(const std::string& value) -> std::vector<std::string>;
  static auto
  filter_python_sources(const std::vector<std::string>& files) -> std::vector<std::string>;
};

} // namespace horcrux::core
