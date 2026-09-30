#pragma once

#include "Constants.h"

#include <atomic>
#include <vector>

namespace maharajah {

enum class HashFlag : int { exact, alpha, beta };

// Always-replace transposition table, shared by all search threads without locks.
// Entries keep MaharajahC's 24-byte size so a table of the same size in MB holds the
// same number of entries in both engines.
class TranspositionTable {
  public:
  static constexpr int no_entry{ 100000 };
  static constexpr int default_mb{ 64 };
  static constexpr int min_mb{ 1 };
  static constexpr int max_mb{ 1024 };

  explicit TranspositionTable(int mb = default_mb) {
    resize(mb);
  }

  // Reallocates the table (size clamped to [min_mb, max_mb]) and clears it.
  void resize(int mb);
  void clear();

  [[nodiscard]] std::size_t entries() const {
    return table_.size();
  }

  // Score stored for `hash_key` if it is deep enough and usable within the window,
  // otherwise no_entry. Mate scores are stored relative to the node (ply).
  [[nodiscard]] int read(u64 hash_key, int alpha, int beta, int depth, int ply) const;
  void write(u64 hash_key, int score, int depth, HashFlag flag, int ply);

  private:
  // The key is stored XORed with the data word (Hyatt's lockless hashing): a read
  // that races a write from another thread fails the key check instead of returning
  // a mix of two entries.
  struct Entry {
    std::atomic<u64> checked_key{ };
    std::atomic<u64> data{ };
    u64 padding{ };
  };
  static_assert(sizeof(Entry) == 24);

  // data word: score in bits 0-31, depth in 32-47, flag in 48-55
  static u64 pack(int score, int depth, HashFlag flag);
  static int unpacked_score(u64 data);
  static int unpacked_depth(u64 data);
  static HashFlag unpacked_flag(u64 data);

  std::vector<Entry> table_;
};

} // namespace maharajah
