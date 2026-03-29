// Horcrux - Rust Adapter
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <string>
#include <vector>

#include "adapter.h"

namespace horcrux::core {

/// @brief Toolchain information for Rust compilation
struct RustToolchain {
  std::string rustc;         ///< Path to rustc compiler (e.g., "rustc")
  std::string cargo;         ///< Path to cargo tool (e.g., "cargo")
  std::string archiver;      ///< Path to archiver (e.g., "ar")
  std::string rustc_version; ///< rustc version string (e.g., "1.76.0")
  std::string edition;       ///< Default Rust edition (e.g., "2021")
};

/// @brief Rust language adapter supporting rust_library, rust_binary, rust_test
///
/// Implements compile → link/archive pipeline with deterministic cache keys.
/// Toolchain detection falls back: HORCRUX_RUSTC env var → rustc on PATH.
class RustAdapter final : public Adapter {
public:
  /// @brief Create a RustAdapter, detecting the Rust toolchain
  /// @return RustAdapter or error if no suitable toolchain found
  static auto create() -> tl::expected<RustAdapter, AdapterError>;

  /// @brief Create a RustAdapter with an explicit toolchain (for testing)
  explicit RustAdapter(RustToolchain toolchain);

  ~RustAdapter() override = default;
  RustAdapter(RustAdapter&&) = default;
  RustAdapter& operator=(RustAdapter&&) = default;

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
  [[nodiscard]] auto toolchain() const -> const RustToolchain& {
    return toolchain_;
  }

private:
  AdapterInfo info_;
  RustToolchain toolchain_;
  mutable std::vector<Diagnostic> diagnostics_;

  void emit(Diagnostic::Level level, std::string message,
            std::optional<std::string> location = std::nullopt) const;

  auto plan_compile_action(const AdapterTarget& target, const std::string& output_root) const
      -> tl::expected<BuildAction, AdapterError>;

  auto
  plan_link_action(const AdapterTarget& target, const BuildGraph& graph,
                   const std::string& output_root) const -> tl::expected<BuildAction, AdapterError>;

  auto plan_test_action(const AdapterTarget& target, const std::string& output_root) const
      -> tl::expected<BuildAction, AdapterError>;

  static auto detect_toolchain() -> tl::expected<RustToolchain, AdapterError>;
  static auto label_target_name(const std::string& label) -> std::string;
  static auto label_to_safe_path(const std::string& label) -> std::string;
  static auto split_tokens(const std::string& value) -> std::vector<std::string>;
  static auto
  filter_rust_sources(const std::vector<std::string>& files) -> std::vector<std::string>;
};

} // namespace horcrux::core
