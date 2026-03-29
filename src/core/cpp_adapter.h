// Horcrux - C++ Adapter MVP
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <string>
#include <vector>

#include "adapter.h"

namespace horcrux::core {

/// @brief Toolchain information for C++ compilation
struct CppToolchain {
  std::string compiler;     ///< Path to compiler (e.g., "/usr/bin/g++")
  std::string archiver;     ///< Path to archiver (e.g., "/usr/bin/ar")
  std::string compiler_id;  ///< Compiler identifier (e.g., "gcc", "clang")
  std::string compiler_version; ///< Compiler version string
};

/// @brief C++ language adapter supporting cc_library, cc_binary, cc_test
///
/// Implements compile → archive/link pipeline with deterministic cache keys.
/// Toolchain detection falls back: HORCRUX_CXX env var → g++ → clang++.
class CppAdapter final : public Adapter {
public:
  /// @brief Create a CppAdapter, detecting the C++ toolchain
  /// @return CppAdapter or error if no suitable toolchain found
  static auto create() -> tl::expected<CppAdapter, AdapterError>;

  /// @brief Create a CppAdapter with an explicit toolchain (for testing)
  explicit CppAdapter(CppToolchain toolchain);

  ~CppAdapter() override = default;
  CppAdapter(CppAdapter&&) = default;
  CppAdapter& operator=(CppAdapter&&) = default;

  [[nodiscard]] auto info() const -> const AdapterInfo& override;

  [[nodiscard]] auto
  parse_target(const BuildNode& node) -> tl::expected<AdapterTarget, AdapterError> override;

  [[nodiscard]] auto
  plan_actions(const AdapterTarget& target, const BuildGraph& graph,
               const std::string& output_root) -> tl::expected<std::vector<BuildAction>, AdapterError> override;

  [[nodiscard]] auto
  compute_cache_key(const AdapterTarget& target) -> tl::expected<Hash, AdapterError> override;

  [[nodiscard]] auto diagnostics() const -> const std::vector<Diagnostic>& override;

  /// @brief Get the detected toolchain
  [[nodiscard]] auto toolchain() const -> const CppToolchain& {
    return toolchain_;
  }

private:
  AdapterInfo info_;
  CppToolchain toolchain_;
  mutable std::vector<Diagnostic> diagnostics_;

  /// @brief Emit a diagnostic message
  void emit(Diagnostic::Level level, std::string message,
            std::optional<std::string> location = std::nullopt) const;

  /// @brief Plan compile actions for source files
  auto plan_compile_actions(const AdapterTarget& target,
                             const std::string& output_root,
                             std::vector<std::string>& obj_files) const
      -> tl::expected<std::vector<BuildAction>, AdapterError>;

  /// @brief Plan archive action (for cc_library)
  auto plan_archive_action(const AdapterTarget& target,
                            const std::string& output_root,
                            const std::vector<std::string>& obj_files) const
      -> tl::expected<BuildAction, AdapterError>;

  /// @brief Plan link action (for cc_binary / cc_test)
  auto plan_link_action(const AdapterTarget& target,
                         const BuildGraph& graph,
                         const std::string& output_root,
                         const std::vector<std::string>& obj_files) const
      -> tl::expected<BuildAction, AdapterError>;

  /// @brief Detect C++ toolchain from environment or PATH
  static auto detect_toolchain() -> tl::expected<CppToolchain, AdapterError>;

  /// @brief Normalize a label-relative path to a filesystem-friendly path
  static auto label_to_path(const std::string& label) -> std::string;

  /// @brief Extract the target name from a label "//pkg:name" → "name"
  static auto label_target_name(const std::string& label) -> std::string;

  /// @brief Split a comma/space-separated attribute value into tokens
  static auto split_attr(const std::string& value) -> std::vector<std::string>;
};

} // namespace horcrux::core
