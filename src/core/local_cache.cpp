// Horcrux - Local Cache System Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "local_cache.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

#include <openssl/evp.h>
#include <tl/expected.hpp>

namespace horcrux::core {

auto compute_sha256(std::span<const uint8_t> data) -> Hash {
  Hash result{};
  auto* ctx = EVP_MD_CTX_new();
  if (ctx == nullptr) {
    return result;
  }

  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(ctx, data.data(), data.size()) != 1) {
    EVP_MD_CTX_free(ctx);
    return result;
  }

  unsigned int len = result.size();
  EVP_DigestFinal_ex(ctx, result.data(), &len);
  EVP_MD_CTX_free(ctx);
  return result;
}

auto hash_to_string(const Hash& hash) -> std::string {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (uint8_t byte : hash) {
    oss << std::setw(2) << static_cast<int>(byte);
  }
  return oss.str();
}

auto mix_policy_fingerprint(const Hash& base, const Hash& policy_fp) -> Hash {
  // Concatenate both hashes and hash the result
  std::vector<uint8_t> combined;
  combined.reserve(base.size() + policy_fp.size());
  combined.insert(combined.end(), base.begin(), base.end());
  combined.insert(combined.end(), policy_fp.begin(), policy_fp.end());
  return compute_sha256(combined);
}

// ── Portable integer I/O helpers (big-endian wire format) ──────────────

namespace {

void write_uint64_be(std::ostream& out, uint64_t value) {
  for (int i = 7; i >= 0; --i) {
    out.put(static_cast<char>((value >> (i * 8)) & 0xff));
  }
}

auto read_uint64_be(std::istream& in) -> uint64_t {
  uint64_t value = 0;
  for (int i = 7; i >= 0; --i) {
    value = (value << 8) | static_cast<uint8_t>(in.get());
  }
  return value;
}

void write_int64_be(std::ostream& out, int64_t value) {
  write_uint64_be(out, static_cast<uint64_t>(value));
}

auto read_int64_be(std::istream& in) -> int64_t {
  return static_cast<int64_t>(read_uint64_be(in));
}

} // anonymous namespace

auto LocalCache::create(const std::filesystem::path& cache_dir)
    -> tl::expected<LocalCache, CacheError> {
  // Create cache directory if it doesn't exist
  std::error_code ec;
  if (!std::filesystem::exists(cache_dir, ec)) {
    if (!std::filesystem::create_directories(cache_dir, ec)) {
      return tl::unexpected(CacheError::WriteFailure);
    }
  }

  return LocalCache(cache_dir);
}

LocalCache::LocalCache(const std::filesystem::path& cache_dir)
    : cache_dir_(cache_dir), mutex_(std::make_unique<std::mutex>()) {
}

auto LocalCache::get_cache_path(const Hash& hash) const -> std::filesystem::path {
  // Use first 2 characters for subdirectory (256 buckets)
  // This helps with filesystem performance for large caches
  std::string hash_str = hash_to_string(hash);
  std::string subdir = hash_str.substr(0, 2);
  return cache_dir_ / subdir / hash_str;
}

auto LocalCache::store(const Hash& hash,
                       const Artifact& artifact) -> tl::expected<void, CacheError> {
  // Write to disk first (lock-free: unique path per hash)
  auto file_path = get_cache_path(hash);
  auto dir_path = file_path.parent_path();

  std::error_code ec;
  if (!std::filesystem::exists(dir_path, ec)) {
    if (!std::filesystem::create_directories(dir_path, ec)) {
      return tl::unexpected(CacheError::WriteFailure);
    }
  }

  std::ofstream out(file_path, std::ios::binary);
  if (!out) {
    return tl::unexpected(CacheError::WriteFailure);
  }

  // Portable big-endian wire format
  write_int64_be(out, artifact.timestamp);
  write_uint64_be(out, artifact.content.size());
  out.write(reinterpret_cast<const char*>(artifact.content.data()),
            static_cast<std::streamsize>(artifact.content.size()));

  if (!out) {
    return tl::unexpected(CacheError::WriteFailure);
  }

  // Update memory cache under lock
  {
    std::scoped_lock lock(*mutex_);
    memory_cache_[hash] = artifact;
  }

  return {};
}

auto LocalCache::lookup(const Hash& hash) const -> std::optional<Artifact> {
  // Check memory cache first (under lock)
  {
    std::scoped_lock lock(*mutex_);
    auto it = memory_cache_.find(hash);
    if (it != memory_cache_.end()) {
      return it->second;
    }
  }

  // Try loading from disk (lock-free)
  auto file_path = get_cache_path(hash);
  if (!std::filesystem::exists(file_path)) {
    return std::nullopt;
  }

  std::ifstream in(file_path, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }

  Artifact artifact;

  // Portable big-endian wire format
  artifact.timestamp = read_int64_be(in);
  if (!in) {
    return std::nullopt;
  }

  auto content_size = read_uint64_be(in);
  if (!in) {
    return std::nullopt;
  }

  artifact.content.resize(static_cast<size_t>(content_size));
  in.read(reinterpret_cast<char*>(artifact.content.data()),
          static_cast<std::streamsize>(content_size));
  if (!in) {
    return std::nullopt;
  }

  // Cache in memory for future lookups (under lock)
  {
    std::scoped_lock lock(*mutex_);
    memory_cache_[hash] = artifact;
  }

  return artifact;
}

auto LocalCache::contains(const Hash& hash) const -> bool {
  // Check memory cache first (under lock)
  {
    std::scoped_lock lock(*mutex_);
    if (memory_cache_.find(hash) != memory_cache_.end()) {
      return true;
    }
  }

  // Check disk (lock-free)
  auto file_path = get_cache_path(hash);
  return std::filesystem::exists(file_path);
}

auto LocalCache::size() const -> size_t {
  std::scoped_lock lock(*mutex_);
  return memory_cache_.size();
}

auto LocalCache::clear() -> tl::expected<void, CacheError> {
  {
    std::scoped_lock lock(*mutex_);
    memory_cache_.clear();
  }

  // Remove all cache files
  std::error_code ec;
  if (std::filesystem::exists(cache_dir_, ec)) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(cache_dir_)) {
      if (entry.is_regular_file()) {
        std::filesystem::remove(entry.path(), ec);
        if (ec) {
          return tl::unexpected(CacheError::WriteFailure);
        }
      }
    }
  }

  return {};
}

} // namespace horcrux::core
