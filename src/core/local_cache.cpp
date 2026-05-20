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

#include <tl/expected.hpp>

// SHA-256 implementation using OpenSSL-like approach
// For production, consider using a proper crypto library
namespace horcrux::core {

namespace {

// Simple SHA-256 implementation for demonstration
// Note: In production, use a well-tested crypto library like OpenSSL or libsodium
constexpr uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

constexpr auto rotr(uint32_t x, uint32_t n) -> uint32_t {
  return (x >> n) | (x << (32 - n));
}

constexpr auto ch(uint32_t x, uint32_t y, uint32_t z) -> uint32_t {
  return (x & y) ^ (~x & z);
}

constexpr auto maj(uint32_t x, uint32_t y, uint32_t z) -> uint32_t {
  return (x & y) ^ (x & z) ^ (y & z);
}

constexpr auto sigma0(uint32_t x) -> uint32_t {
  return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

constexpr auto sigma1(uint32_t x) -> uint32_t {
  return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

constexpr auto gamma0(uint32_t x) -> uint32_t {
  return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

constexpr auto gamma1(uint32_t x) -> uint32_t {
  return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
  uint32_t w[64];

  // Prepare message schedule
  for (int i = 0; i < 16; ++i) {
    w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
           (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
           (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
           (static_cast<uint32_t>(block[i * 4 + 3]));
  }

  for (int i = 16; i < 64; ++i) {
    w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
  }

  // Initialize working variables
  uint32_t a = state[0];
  uint32_t b = state[1];
  uint32_t c = state[2];
  uint32_t d = state[3];
  uint32_t e = state[4];
  uint32_t f = state[5];
  uint32_t g = state[6];
  uint32_t h = state[7];

  // Main loop
  for (int i = 0; i < 64; ++i) {
    uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + w[i];
    uint32_t t2 = sigma0(a) + maj(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }

  // Add working variables back into state
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  state[5] += f;
  state[6] += g;
  state[7] += h;
}

} // anonymous namespace

auto compute_sha256(std::span<const uint8_t> data) -> Hash {
  // Initial hash values (first 32 bits of fractional parts of square roots of
  // first 8 primes)
  uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                       0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

  size_t total_len = data.size();
  size_t processed = 0;

  // Process 64-byte blocks
  while (processed + 64 <= total_len) {
    sha256_transform(state, data.data() + processed);
    processed += 64;
  }

  // Prepare final block with padding
  uint8_t final_block[128] = {};
  size_t remaining = total_len - processed;

  // Copy remaining bytes
  std::memcpy(final_block, data.data() + processed, remaining);

  // Append '1' bit (0x80)
  final_block[remaining] = 0x80;

  // If not enough space for length, process this block and start a new one
  size_t block_count = 1;
  if (remaining >= 56) {
    block_count = 2;
  }

  // Append length in bits as 64-bit big-endian integer
  uint64_t bit_len = total_len * 8;
  size_t len_offset = block_count * 64 - 8;
  for (int i = 0; i < 8; ++i) {
    final_block[len_offset + i] = static_cast<uint8_t>((bit_len >> (56 - i * 8)) & 0xff);
  }

  // Process final block(s)
  for (size_t i = 0; i < block_count; ++i) {
    sha256_transform(state, final_block + i * 64);
  }

  // Convert state to byte array (big-endian)
  Hash result{};
  for (int i = 0; i < 8; ++i) {
    result[i * 4] = static_cast<uint8_t>((state[i] >> 24) & 0xff);
    result[i * 4 + 1] = static_cast<uint8_t>((state[i] >> 16) & 0xff);
    result[i * 4 + 2] = static_cast<uint8_t>((state[i] >> 8) & 0xff);
    result[i * 4 + 3] = static_cast<uint8_t>(state[i] & 0xff);
  }

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
