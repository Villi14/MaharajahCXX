#pragma once

#include "Types.h"

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

  struct Probe {
    // usable score, or no_entry
    int score{ no_entry };
    // best move stored for the position, 0 if none
    int move{ };
  };

  explicit TranspositionTable(int mb = default_mb) {
    resize(mb);
  }

  // Reallocates the table (size clamped to [min_mb, max_mb]) and clears it.
  void resize(int mb);
  void clear();

  [[nodiscard]] std::size_t entries() const {
    return table_.size();
  }

  // The score stored for `hash_key` if it is deep enough and usable within the window
  // (otherwise no_entry), and the best move stored for it at any depth. Mate scores
  // are stored relative to the node (ply).
  [[nodiscard]] Probe probe(u64 hash_key, int alpha, int beta, int depth, int ply) const;
  [[nodiscard]] int read(const u64 hash_key, const int alpha, const int beta, const int depth, const int ply) const {
    return probe(hash_key, alpha, beta, depth, ply).score;
  }
  // `move` 0 (a node that failed low) keeps the move already stored for the position.
  void write(u64 hash_key, int score, int depth, HashFlag flag, int ply, int move = 0);

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

  // data word: score in bits 0-23, depth in 24-31, flag in 32-33, move in 34-59
  static u64 pack(int score, int depth, HashFlag flag, int move);
  static int unpacked_score(u64 data);
  static int unpacked_depth(u64 data);
  static HashFlag unpacked_flag(u64 data);
  static int unpacked_move(u64 data);

  std::vector<Entry> table_;
};

} // namespace maharajah
