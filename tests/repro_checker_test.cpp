// Horcrux - ReproChecker Unit Tests
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/core/local_cache.h"
#include "../src/core/repro_checker.h"

namespace horcrux::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// Helper: temporary directory
// ─────────────────────────────────────────────────────────────────────────────

class TempDir {
public:
  TempDir() {
    // Use a monotonic counter for uniqueness across parallel test instances
    static std::atomic<int> counter{0};
    int id = counter.fetch_add(1);
    path_ = std::filesystem::temp_directory_path() /
            ("horcrux_repro_test_" + std::to_string(id));
    std::filesystem::create_directories(path_);
  }
  ~TempDir() {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }
  auto path() const -> const std::filesystem::path& {
    return path_;
  }

private:
  std::filesystem::path path_;
};

// ─────────────────────────────────────────────────────────────────────────────
// to_string(ReproError)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ReproErrorTest, ToStringCoversAllValues) {
  EXPECT_FALSE(to_string(ReproError::BuildFailed).empty());
  EXPECT_FALSE(to_string(ReproError::ArtifactMissing).empty());
  EXPECT_FALSE(to_string(ReproError::HashComputationError).empty());
  EXPECT_FALSE(to_string(ReproError::IoError).empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// compare_artifacts
// ─────────────────────────────────────────────────────────────────────────────

TEST(CompareArtifactsTest, MatchingHashesProducesReproducibleReport) {
  std::vector<std::string> paths = {"libhello.a", "hello"};
  std::vector<uint8_t> data_a = {'a', 'b', 'c'};
  std::vector<uint8_t> data_b = {'d', 'e', 'f'};
  auto h_a = compute_sha256(data_a);
  auto h_b = compute_sha256(data_b);

  std::vector<Hash> hashes1 = {h_a, h_b};
  std::vector<Hash> hashes2 = {h_a, h_b}; // Same as round 1

  auto report = compare_artifacts(paths, hashes1, hashes2);
  EXPECT_TRUE(report.reproducible);
  EXPECT_TRUE(report.differing_artifacts().empty());
}

TEST(CompareArtifactsTest, DifferentHashesProducesNonReproducibleReport) {
  std::vector<std::string> paths = {"libhello.a"};
  std::vector<uint8_t> data1 = {'a', 'b', 'c'};
  std::vector<uint8_t> data2 = {'x', 'y', 'z'};
  auto h1 = compute_sha256(data1);
  auto h2 = compute_sha256(data2);

  auto report = compare_artifacts(paths, {h1}, {h2});
  EXPECT_FALSE(report.reproducible);
  ASSERT_EQ(report.differing_artifacts().size(), 1u);
  EXPECT_EQ(report.differing_artifacts()[0], "libhello.a");
}

TEST(CompareArtifactsTest, EmptyPathsProducesReproducibleReport) {
  auto report = compare_artifacts({}, {}, {});
  EXPECT_TRUE(report.reproducible);
  EXPECT_TRUE(report.diffs.empty());
}

TEST(CompareArtifactsTest, ReportSummaryContainsPassOrFail) {
  std::vector<std::string> paths = {"out.o"};
  std::vector<uint8_t> data = {'a'};
  auto h = compute_sha256(data);
  auto report = compare_artifacts(paths, {h}, {h});
  auto summary = report.summary();
  EXPECT_NE(summary.find("PASS"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// ReproChecker – in-memory round recording
// ─────────────────────────────────────────────────────────────────────────────

TEST(ReproCheckerTest, IdenticalRoundsAreReproducible) {
  std::vector<uint8_t> data = {'b', 'u', 'i', 'l', 't'};
  auto h = compute_sha256(data);

  ReproChecker checker;
  checker.record_round_from_map({{"app", h}}, 1);
  checker.record_round_from_map({{"app", h}}, 2);

  auto report = checker.check();
  EXPECT_TRUE(report.reproducible);
}

TEST(ReproCheckerTest, DifferentRoundsAreNonReproducible) {
  std::vector<uint8_t> data1 = {'b', 'u', 'i', 'l', 't'};
  std::vector<uint8_t> data2 = {'X', 'Y', 'Z'};
  auto h1 = compute_sha256(data1);
  auto h2 = compute_sha256(data2);

  ReproChecker checker;
  checker.record_round_from_map({{"app", h1}}, 1);
  checker.record_round_from_map({{"app", h2}}, 2);

  auto report = checker.check();
  EXPECT_FALSE(report.reproducible);
  ASSERT_EQ(report.differing_artifacts().size(), 1u);
  EXPECT_EQ(report.differing_artifacts()[0], "app");
}

TEST(ReproCheckerTest, ResetClearsState) {
  std::vector<uint8_t> data = {'x'};
  auto h = compute_sha256(data);

  ReproChecker checker;
  checker.record_round_from_map({{"app", h}}, 1);
  checker.record_round_from_map({{"app", h}}, 2);

  checker.reset();

  // After reset both rounds are empty → all-empty is reproducible
  auto report = checker.check();
  EXPECT_TRUE(report.reproducible);
  EXPECT_TRUE(report.diffs.empty());
}

TEST(ReproCheckerTest, SortingOfInputMapIsStable) {
  std::vector<uint8_t> d1 = {'a'};
  std::vector<uint8_t> d2 = {'b'};
  auto h1 = compute_sha256(d1);
  auto h2 = compute_sha256(d2);

  ReproChecker checker;
  // Record in different insertion order in rounds 1 and 2
  checker.record_round_from_map({{"z_out", h1}, {"a_out", h2}}, 1);
  checker.record_round_from_map({{"a_out", h2}, {"z_out", h1}}, 2);

  auto report = checker.check();
  EXPECT_TRUE(report.reproducible);
}

TEST(ReproCheckerTest, ArtifactPresentOnlyInRound2IsDetected) {
  std::vector<uint8_t> data = {'x'};
  auto h = compute_sha256(data);
  Hash zero_hash{};

  ReproChecker checker;
  checker.record_round_from_map({}, 1);                     // Round 1: no artifacts
  checker.record_round_from_map({{"new_file", h}}, 2);      // Round 2: has artifact

  auto report = checker.check();
  // Artifact present only in round 2 means round 1 has a zero hash → not reproducible
  EXPECT_FALSE(report.reproducible);
}

// ─────────────────────────────────────────────────────────────────────────────
// hash_file
// ─────────────────────────────────────────────────────────────────────────────

TEST(HashFileTest, HashesAFileCorrectly) {
  TempDir tmp;
  auto file_path = tmp.path() / "test.bin";

  // Write known content
  std::vector<uint8_t> content = {'h', 'e', 'l', 'l', 'o'};
  {
    std::ofstream out(file_path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(content.data()),
              static_cast<std::streamsize>(content.size()));
  }

  auto result = hash_file(file_path);
  ASSERT_TRUE(result.has_value());

  auto expected = compute_sha256(content);
  EXPECT_EQ(*result, expected);
}

TEST(HashFileTest, ReturnsErrorForMissingFile) {
  auto result = hash_file("/tmp/this_file_does_not_exist_horcrux_test_xyz");
  EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// ReproChecker – disk round recording
// ─────────────────────────────────────────────────────────────────────────────

TEST(ReproCheckerTest, RecordRoundFromDiskDetectsIdenticalOutputs) {
  TempDir out1;
  TempDir out2;

  // Write the same content to both output directories
  auto write_file = [](const std::filesystem::path& dir, const std::string& name,
                       const std::string& content) {
    std::ofstream f(dir / name);
    f << content;
  };

  write_file(out1.path(), "app.o", "object_code_v1");
  write_file(out2.path(), "app.o", "object_code_v1"); // Identical

  ReproChecker checker;
  ASSERT_TRUE(checker.record_round(out1.path(), 1).has_value());
  ASSERT_TRUE(checker.record_round(out2.path(), 2).has_value());

  auto report = checker.check();
  EXPECT_TRUE(report.reproducible);
}

TEST(ReproCheckerTest, RecordRoundFromDiskDetectsDifferentOutputs) {
  TempDir out1;
  TempDir out2;

  auto write_file = [](const std::filesystem::path& dir, const std::string& name,
                       const std::string& content) {
    std::ofstream f(dir / name);
    f << content;
  };

  write_file(out1.path(), "app.o", "object_code_v1");
  write_file(out2.path(), "app.o", "object_code_v2_different"); // Different

  ReproChecker checker;
  ASSERT_TRUE(checker.record_round(out1.path(), 1).has_value());
  ASSERT_TRUE(checker.record_round(out2.path(), 2).has_value());

  auto report = checker.check();
  EXPECT_FALSE(report.reproducible);
  EXPECT_FALSE(report.differing_artifacts().empty());
}

TEST(ReproCheckerTest, RecordRoundFailsForMissingDirectory) {
  ReproChecker checker;
  auto result = checker.record_round("/tmp/this_dir_does_not_exist_horcrux_xyz_test", 1);
  EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// ReproReport – non_hermetic_hints
// ─────────────────────────────────────────────────────────────────────────────

TEST(ReproReportTest, NoHintsWhenReproducible) {
  auto report = compare_artifacts({}, {}, {});
  EXPECT_TRUE(report.non_hermetic_hints().empty());
}

TEST(ReproReportTest, HintsProvidedWhenNonReproducible) {
  std::vector<uint8_t> d1 = {'a'}, d2 = {'b'};
  auto h1 = compute_sha256(d1);
  auto h2 = compute_sha256(d2);

  auto report = compare_artifacts({"out.o"}, {h1}, {h2});
  EXPECT_FALSE(report.non_hermetic_hints().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ArtifactDiff hex strings
// ─────────────────────────────────────────────────────────────────────────────

TEST(ArtifactDiffTest, HexStringsAre64Chars) {
  std::vector<uint8_t> data = {'x'};
  auto h = compute_sha256(data);
  ArtifactDiff diff{"path", h, h, true};
  EXPECT_EQ(diff.hash1_hex().size(), 64u);
  EXPECT_EQ(diff.hash2_hex().size(), 64u);
}

} // namespace horcrux::core::test
