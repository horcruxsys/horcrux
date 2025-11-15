// Horcrux - Local Cache System
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace horcrux::core {

/// SHA-256 hash represented as 32 bytes
using Hash = std::array<uint8_t, 32>;

/// Hash function for std::array<uint8_t, 32>
struct HashHasher {
  auto operator()(const Hash& hash) const -> size_t {
    size_t result = 0;
    // Combine first 8 bytes into size_t
    for (size_t i = 0; i < 8 && i < hash.size(); ++i) {
      result ^= static_cast<size_t>(hash[i]) << (i * 8);
    }
    return result;
  }
};

/// Error types for cache operations
enum class CacheError {
  FileNotFound,
  WriteFailure,
  ReadFailure,
  InvalidHash,
  CorruptedData
};

/// Artifact stored in the cache
struct Artifact {
  std::vector<uint8_t> content;
  int64_t timestamp;

  auto operator==(const Artifact& other) const -> bool = default;
};

/// Computes SHA-256 hash of the given data
///
/// @param data Input data to hash
/// @return SHA-256 hash as 32-byte array
///
/// @complexity O(n) where n is the size of data
/// @note This function is deterministic and thread-safe
auto compute_sha256(std::span<const uint8_t> data) -> Hash;

/// Converts a hash to hexadecimal string representation
///
/// @param hash Hash to convert
/// @return Hexadecimal string (64 characters)
auto hash_to_string(const Hash& hash) -> std::string;

/// LocalCache provides content-addressable storage for build artifacts
///
/// The cache stores artifacts using SHA-256 hashes as keys. Each artifact
/// is stored at a path derived from its hash, ensuring deterministic
/// storage and retrieval.
///
/// Thread-safety: All methods are thread-safe and can be called concurrently.
class LocalCache {
public:
  /// Creates a new LocalCache with the specified cache directory
  ///
  /// @param cache_dir Directory where cache files will be stored
  /// @return Expected LocalCache or CacheError
  ///
  /// @note Creates the cache directory if it doesn't exist
  static auto create(const std::filesystem::path& cache_dir)
      -> std::expected<LocalCache, CacheError>;

  /// Stores an artifact in the cache
  ///
  /// @param hash Content hash (SHA-256)
  /// @param artifact Artifact data to store
  /// @return Expected void or CacheError on failure
  ///
  /// @complexity O(n) where n is the size of artifact content
  /// @note Thread-safe operation
  auto store(const Hash& hash, const Artifact& artifact)
      -> std::expected<void, CacheError>;

  /// Retrieves an artifact from the cache
  ///
  /// @param hash Content hash (SHA-256)
  /// @return Optional artifact if found, nullopt otherwise
  ///
  /// @complexity O(1) for memory lookup, O(n) for disk read
  /// @note Thread-safe operation
  auto lookup(const Hash& hash) const -> std::optional<Artifact>;

  /// Checks if an artifact exists in the cache
  ///
  /// @param hash Content hash to check
  /// @return true if artifact exists, false otherwise
  ///
  /// @complexity O(1)
  /// @note Thread-safe operation
  auto contains(const Hash& hash) const -> bool;

  /// Returns the number of artifacts in the cache
  ///
  /// @return Number of cached artifacts
  auto size() const -> size_t;

  /// Clears all artifacts from the cache
  ///
  /// @return Expected void or CacheError on failure
  auto clear() -> std::expected<void, CacheError>;

private:
  explicit LocalCache(const std::filesystem::path& cache_dir);

  /// Computes the filesystem path for a given hash
  auto get_cache_path(const Hash& hash) const -> std::filesystem::path;

  std::filesystem::path cache_dir_;
  mutable std::unordered_map<Hash, Artifact, HashHasher> memory_cache_;
};

} // namespace horcrux::core

// Specialize std::hash for Hash type (optional, for compatibility)
namespace std {
template <>
struct hash<horcrux::core::Hash> {
  auto operator()(const horcrux::core::Hash& h) const -> size_t {
    return horcrux::core::HashHasher{}(h);
  }
};
} // namespace std
