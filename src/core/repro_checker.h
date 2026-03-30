// Horcrux - Reproducibility Checker
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "local_cache.h"
#include "sandbox_policy.h"

namespace horcrux::core {

/// @brief Error types for reproducibility check operations
enum class ReproError {
  BuildFailed,          ///< One or both build rounds failed
  ArtifactMissing,      ///< An expected output artifact was not found
  HashComputationError, ///< Could not hash an output file
  IoError,              ///< Filesystem I/O error
};

/// @brief Convert ReproError to human-readable string
[[nodiscard]] auto to_string(ReproError err) -> std::string;

/// @brief Per-artifact comparison result
struct ArtifactDiff {
  std::string path;    ///< Relative artifact path
  Hash hash1;          ///< Hash from build round 1
  Hash hash2;          ///< Hash from build round 2
  bool match;          ///< true iff hash1 == hash2

  /// Human-readable hex strings
  [[nodiscard]] auto hash1_hex() const -> std::string;
  [[nodiscard]] auto hash2_hex() const -> std::string;
};

/// @brief Summary report produced by a repro-check run
struct ReproReport {
  bool reproducible;                   ///< true iff all artifacts matched
  std::vector<ArtifactDiff> diffs;     ///< Per-artifact comparison results

  /// Paths of artifacts that differed between builds
  [[nodiscard]] auto differing_artifacts() const -> std::vector<std::string>;

  /// Hints about suspected non-hermetic inputs based on diff patterns
  [[nodiscard]] auto non_hermetic_hints() const -> std::vector<std::string>;

  /// Human-readable multi-line summary
  [[nodiscard]] auto summary() const -> std::string;
};

/// @brief Computes the SHA-256 hash of a file on disk
///
/// @param path Path to the file
/// @return Hash or ReproError on failure
[[nodiscard]] auto hash_file(const std::filesystem::path& path)
    -> tl::expected<Hash, ReproError>;

/// @brief Compares two sets of artifact hashes and builds a diff report
///
/// @param paths    Relative artifact paths to compare
/// @param hashes1  Hashes from build round 1 (same order as paths)
/// @param hashes2  Hashes from build round 2 (same order as paths)
/// @return ReproReport
[[nodiscard]] auto compare_artifacts(const std::vector<std::string>& paths,
                                     const std::vector<Hash>& hashes1,
                                     const std::vector<Hash>& hashes2) -> ReproReport;

/// @brief ReproChecker orchestrates the double-build reproducibility check
///
/// In repro-check mode the build system:
///   1. Executes a clean build and captures output artifact hashes.
///   2. Executes a second clean build with the same inputs and captures hashes.
///   3. Compares the two sets of hashes and emits a ReproReport.
///
/// This class is intentionally simple: it does not invoke the build itself
/// (that responsibility belongs to BuildExecutor). Instead it manages the
/// hash capture and comparison logic so it can be unit-tested in isolation.
class ReproChecker {
public:
  /// @brief Record artifact hashes from a completed build round
  ///
  /// Scans @p output_dir for all regular files and computes their SHA-256
  /// hashes. The paths stored are relative to @p output_dir.
  ///
  /// @param output_dir  Directory containing build outputs
  /// @param round       Build round index (1 or 2)
  /// @return Expected void or ReproError
  auto record_round(const std::filesystem::path& output_dir, int round)
      -> tl::expected<void, ReproError>;

  /// @brief Record hashes directly from an in-memory map (useful for testing)
  ///
  /// @param hashes  Map from relative path → hash
  /// @param round   Build round index (1 or 2)
  void record_round_from_map(const std::vector<std::pair<std::string, Hash>>& hashes, int round);

  /// @brief Compare round 1 and round 2 recordings and produce a report
  ///
  /// Both rounds must have been recorded before calling this method.
  ///
  /// @return ReproReport
  [[nodiscard]] auto check() const -> ReproReport;

  /// @brief Reset all recorded state (useful when re-using across multiple targets)
  void reset();

private:
  std::vector<std::string> paths1_;
  std::vector<Hash> hashes1_;
  std::vector<std::string> paths2_;
  std::vector<Hash> hashes2_;
};

} // namespace horcrux::core
