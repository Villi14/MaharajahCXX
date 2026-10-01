#include "../headers/Transposition.h"
#include "../headers/Constants.h"

#include <algorithm>
#include <cstdint>

namespace maharajah {

namespace {

constexpr int score_bits{ 24 };
constexpr int depth_shift{ 24 };
constexpr int flag_shift{ 32 };
constexpr int move_shift{ 34 };
constexpr u64 move_mask{ 0x3FFFFFF };

} // namespace

void TranspositionTable::resize(const int mb) {
  const std::size_t bytes = std::size_t{ 0x100000 } * static_cast<std::size_t>(std::clamp(mb, min_mb, max_mb));
  table_ = std::vector<Entry>(bytes / sizeof(Entry));
}

void TranspositionTable::clear() {
  for(Entry& entry : table_) {
    entry.checked_key.store(0, std::memory_order_relaxed);
    entry.data.store(0, std::memory_order_relaxed);
  }
}

u64 TranspositionTable::pack(const int score, const int depth, const HashFlag flag, const int move) {
  return (u64{ static_cast<std::uint32_t>(score) } & ((u64{ 1 } << score_bits) - 1)) |
         (u64{ static_cast<std::uint8_t>(std::clamp(depth, 0, 0xFF)) } << depth_shift) |
         (u64{ static_cast<std::uint8_t>(flag) } << flag_shift) | ((u64{ static_cast<std::uint32_t>(move) } & move_mask) << move_shift);
}

int TranspositionTable::unpacked_score(const u64 data) {
  // sign-extend the low 24 bits
  return static_cast<std::int32_t>(static_cast<std::uint32_t>(data) << (32 - score_bits)) >> (32 - score_bits);
}

int TranspositionTable::unpacked_depth(const u64 data) {
  return static_cast<std::uint8_t>(data >> depth_shift);
}

HashFlag TranspositionTable::unpacked_flag(const u64 data) {
  return static_cast<HashFlag>((data >> flag_shift) & 0x3);
}

int TranspositionTable::unpacked_move(const u64 data) {
  return static_cast<int>((data >> move_shift) & move_mask);
}

TranspositionTable::Probe TranspositionTable::probe(const u64 hash_key, const int alpha, const int beta, const int depth, const int ply) const {
  if(table_.empty())
    return { };

  const Entry& entry = table_[hash_key % table_.size()];
  const u64 data = entry.data.load(std::memory_order_relaxed);

  if((entry.checked_key.load(std::memory_order_relaxed) ^ data) != hash_key)
    return { };

  Probe result{ .move = unpacked_move(data) };
  if(unpacked_depth(data) < depth)
    return result;

  int score = unpacked_score(data);
  const HashFlag flag = unpacked_flag(data);
  if(score < -Scores::mate_score)
    score += ply;
  if(score > Scores::mate_score)
    score -= ply;

  if(flag == HashFlag::exact)
    result.score = score;
  else if(flag == HashFlag::alpha && score <= alpha)
    result.score = alpha;
  else if(flag == HashFlag::beta && score >= beta)
    result.score = beta;

  return result;
}

void TranspositionTable::write(const u64 hash_key, int score, const int depth, const HashFlag flag, const int ply, int move) {
  if(table_.empty())
    return;

  Entry& entry = table_[hash_key % table_.size()];

  if(move == 0) {
    const u64 old_data = entry.data.load(std::memory_order_relaxed);
    if((entry.checked_key.load(std::memory_order_relaxed) ^ old_data) == hash_key)
      move = unpacked_move(old_data);
  }

  if(score < -Scores::mate_score)
    score -= ply;
  if(score > Scores::mate_score)
    score += ply;

  const u64 data = pack(score, depth, flag, move);
  entry.checked_key.store(hash_key ^ data, std::memory_order_relaxed);
  entry.data.store(data, std::memory_order_relaxed);
}

} // namespace maharajah
