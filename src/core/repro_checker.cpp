// Horcrux - Reproducibility Checker Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "repro_checker.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>

namespace horcrux::core {

// ─────────────────────────────────────────────────────────────────────────────
// Error helpers
// ─────────────────────────────────────────────────────────────────────────────

auto to_string(ReproError err) -> std::string {
  switch (err) {
  case ReproError::BuildFailed:
    return "Build round failed";
  case ReproError::ArtifactMissing:
    return "Expected artifact is missing";
  case ReproError::HashComputationError:
    return "Failed to compute artifact hash";
  case ReproError::IoError:
    return "I/O error during repro check";
  }
  return "Unknown repro error";
}

// ─────────────────────────────────────────────────────────────────────────────
// ArtifactDiff
// ─────────────────────────────────────────────────────────────────────────────

auto ArtifactDiff::hash1_hex() const -> std::string {
  return hash_to_string(hash1);
}

auto ArtifactDiff::hash2_hex() const -> std::string {
  return hash_to_string(hash2);
}

// ─────────────────────────────────────────────────────────────────────────────
// ReproReport
// ─────────────────────────────────────────────────────────────────────────────

auto ReproReport::differing_artifacts() const -> std::vector<std::string> {
  std::vector<std::string> result;
  for (const auto& d : diffs) {
    if (!d.match) {
      result.push_back(d.path);
    }
  }
  return result;
}

auto ReproReport::non_hermetic_hints() const -> std::vector<std::string> {
  std::vector<std::string> hints;
  auto differing = differing_artifacts();
  if (differing.empty()) {
    return hints;
  }

  hints.push_back(
      "Some build artifacts differed between rounds, suggesting non-hermetic inputs.");

  // Heuristic: embedded timestamps are a common cause
  bool any_timestamp_suspect = false;
  for (const auto& p : differing) {
    // Files with small size differences or debug info paths often contain timestamps
    if (p.find("debug") != std::string::npos || p.find("pdb") != std::string::npos ||
        p.find(".d") != std::string::npos) {
      any_timestamp_suspect = true;
    }
  }
  if (any_timestamp_suspect) {
    hints.push_back(
        "Hint: Embedded timestamps detected in debug/dependency files. "
        "Consider passing -ffile-prefix-map or SOURCE_DATE_EPOCH.");
  }

  hints.push_back(
      "Hint: Run with --sandbox=strict and check for undeclared host tool or env-var access.");
  hints.push_back(
      "Hint: Verify that all actions use deterministic output file naming (no PIDs/random UUIDs).");

  return hints;
}

auto ReproReport::summary() const -> std::string {
  std::ostringstream oss;
  oss << "Reproducibility check: " << (reproducible ? "PASS" : "FAIL") << "\n";
  oss << "  Artifacts compared: " << diffs.size() << "\n";

  auto differing = differing_artifacts();
  oss << "  Artifacts matched:  " << (diffs.size() - differing.size()) << "\n";
  oss << "  Artifacts differed: " << differing.size() << "\n";

  if (!differing.empty()) {
    oss << "\nDiffering artifacts:\n";
    for (const auto& p : differing) {
      oss << "  - " << p << "\n";
    }
    oss << "\nDiagnostics:\n";
    for (const auto& h : non_hermetic_hints()) {
      oss << "  " << h << "\n";
    }
  }
  return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// hash_file
// ─────────────────────────────────────────────────────────────────────────────

auto hash_file(const std::filesystem::path& path) -> tl::expected<Hash, ReproError> {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return tl::unexpected(ReproError::IoError);
  }

  std::vector<uint8_t> content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
  if (file.bad()) {
    return tl::unexpected(ReproError::HashComputationError);
  }

  return compute_sha256(content);
}

// ─────────────────────────────────────────────────────────────────────────────
// compare_artifacts
// ─────────────────────────────────────────────────────────────────────────────

auto compare_artifacts(const std::vector<std::string>& paths,
                       const std::vector<Hash>& hashes1,
                       const std::vector<Hash>& hashes2) -> ReproReport {
  ReproReport report;
  report.reproducible = true;

  size_t n = paths.size();
  for (size_t i = 0; i < n; ++i) {
    ArtifactDiff diff;
    diff.path = paths[i];
    diff.hash1 = (i < hashes1.size()) ? hashes1[i] : Hash{};
    diff.hash2 = (i < hashes2.size()) ? hashes2[i] : Hash{};
    diff.match = (diff.hash1 == diff.hash2);
    if (!diff.match) {
      report.reproducible = false;
    }
    report.diffs.push_back(std::move(diff));
  }
  return report;
}

// ─────────────────────────────────────────────────────────────────────────────
// ReproChecker
// ─────────────────────────────────────────────────────────────────────────────

auto ReproChecker::record_round(const std::filesystem::path& output_dir,
                                int round) -> tl::expected<void, ReproError> {
  if (!std::filesystem::exists(output_dir)) {
    return tl::unexpected(ReproError::IoError);
  }

  std::vector<std::string> paths;
  std::vector<Hash> hashes;

  std::error_code ec;
  for (const auto& entry :
       std::filesystem::recursive_directory_iterator(output_dir, ec)) {
    if (ec) {
      return tl::unexpected(ReproError::IoError);
    }
    if (!entry.is_regular_file()) {
      continue;
    }
    auto rel = std::filesystem::relative(entry.path(), output_dir, ec);
    if (ec) {
      continue;
    }
    auto hash_result = hash_file(entry.path());
    if (!hash_result) {
      return tl::unexpected(hash_result.error());
    }
    paths.push_back(rel.string());
    hashes.push_back(*hash_result);
  }

  // Sort by path for deterministic ordering
  std::vector<size_t> idx(paths.size());
  std::iota(idx.begin(), idx.end(), 0);
  std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
    return paths[a] < paths[b];
  });

  std::vector<std::string> sorted_paths;
  std::vector<Hash> sorted_hashes;
  sorted_paths.reserve(paths.size());
  sorted_hashes.reserve(hashes.size());
  for (size_t i : idx) {
    sorted_paths.push_back(std::move(paths[i]));
    sorted_hashes.push_back(hashes[i]);
  }

  if (round == 1) {
    paths1_ = std::move(sorted_paths);
    hashes1_ = std::move(sorted_hashes);
  } else {
    paths2_ = std::move(sorted_paths);
    hashes2_ = std::move(sorted_hashes);
  }
  return {};
}

void ReproChecker::record_round_from_map(
    const std::vector<std::pair<std::string, Hash>>& hashes, int round) {
  std::vector<std::pair<std::string, Hash>> sorted = hashes;
  std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
    return a.first < b.first;
  });

  std::vector<std::string> paths;
  std::vector<Hash> hs;
  paths.reserve(sorted.size());
  hs.reserve(sorted.size());
  for (auto& [p, h] : sorted) {
    paths.push_back(p);
    hs.push_back(h);
  }

  if (round == 1) {
    paths1_ = std::move(paths);
    hashes1_ = std::move(hs);
  } else {
    paths2_ = std::move(paths);
    hashes2_ = std::move(hs);
  }
}

auto ReproChecker::check() const -> ReproReport {
  // Merge the two path sets – include all paths from both rounds
  std::vector<std::string> all_paths;
  {
    std::set<std::string> path_set(paths1_.begin(), paths1_.end());
    path_set.insert(paths2_.begin(), paths2_.end());
    all_paths.assign(path_set.begin(), path_set.end());
    std::sort(all_paths.begin(), all_paths.end());
  }

  // Build lookup maps
  std::unordered_map<std::string, Hash> map1, map2;
  for (size_t i = 0; i < paths1_.size(); ++i) {
    map1[paths1_[i]] = hashes1_[i];
  }
  for (size_t i = 0; i < paths2_.size(); ++i) {
    map2[paths2_[i]] = hashes2_[i];
  }

  std::vector<Hash> h1, h2;
  h1.reserve(all_paths.size());
  h2.reserve(all_paths.size());
  for (const auto& p : all_paths) {
    h1.push_back(map1.count(p) ? map1[p] : Hash{});
    h2.push_back(map2.count(p) ? map2[p] : Hash{});
  }

  return compare_artifacts(all_paths, h1, h2);
}

void ReproChecker::reset() {
  paths1_.clear();
  hashes1_.clear();
  paths2_.clear();
  hashes2_.clear();
}

} // namespace horcrux::core
