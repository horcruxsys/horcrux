// Horcrux - Java Adapter (non-Android)
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <string>
#include <vector>

#include "adapter.h"

namespace horcrux::core {

/// @brief Toolchain information for Java compilation
struct JavaToolchain {
  std::string javac;          ///< Path to Java compiler (e.g., "javac")
  std::string java;           ///< Path to Java runtime (e.g., "java")
  std::string jar_tool;       ///< Path to jar tool (e.g., "jar")
  std::string java_version;   ///< Java version string (e.g., "17.0.0")
  std::string source_version; ///< Source compatibility (e.g., "17")
  std::string target_version; ///< Target compatibility (e.g., "17")
};

/// @brief Java language adapter (non-Android) supporting java_library, java_binary, java_test
///
/// Implements compile → JAR packaging pipeline with deterministic cache keys
/// and classpath resolution from dependency closure.
/// Toolchain detection falls back: HORCRUX_JAVAC env var → javac on PATH.
class JavaAdapter final : public Adapter {
public:
  /// @brief Create a JavaAdapter, detecting the Java toolchain
  /// @return JavaAdapter or error if no suitable toolchain found
  static auto create() -> tl::expected<JavaAdapter, AdapterError>;

  /// @brief Create a JavaAdapter with an explicit toolchain (for testing)
  explicit JavaAdapter(JavaToolchain toolchain);

  ~JavaAdapter() override = default;
  JavaAdapter(JavaAdapter&&) = default;
  JavaAdapter& operator=(JavaAdapter&&) = default;

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
  [[nodiscard]] auto toolchain() const -> const JavaToolchain& {
    return toolchain_;
  }

private:
  AdapterInfo info_;
  JavaToolchain toolchain_;
  mutable std::vector<Diagnostic> diagnostics_;

  void emit(Diagnostic::Level level, std::string message,
            std::optional<std::string> location = std::nullopt) const;

  auto plan_compile_action(const AdapterTarget& target, const BuildGraph& graph,
                           const std::string& output_root) const
      -> tl::expected<BuildAction, AdapterError>;

  auto plan_jar_action(const AdapterTarget& target, const std::string& output_root) const
      -> tl::expected<BuildAction, AdapterError>;

  auto plan_test_action(const AdapterTarget& target, const std::string& output_root) const
      -> tl::expected<BuildAction, AdapterError>;

  static auto detect_toolchain() -> tl::expected<JavaToolchain, AdapterError>;
  static auto label_target_name(const std::string& label) -> std::string;
  static auto label_to_safe_path(const std::string& label) -> std::string;
  static auto split_tokens(const std::string& value) -> std::vector<std::string>;
  static auto
  filter_java_sources(const std::vector<std::string>& files) -> std::vector<std::string>;
  static auto collect_classpath(const AdapterTarget& target, const BuildGraph& graph,
                                const std::string& output_root) -> std::vector<std::string>;
};

} // namespace horcrux::core
