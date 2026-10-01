#include "../headers/Transposition.h"
#include "../headers/Constants.h"

#include <algorithm>
#include <cstdint>

namespace maharajah {

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

u64 TranspositionTable::pack(const int score, const int depth, const HashFlag flag) {
  return u64{ static_cast<std::uint32_t>(score) } | (u64{ static_cast<std::uint16_t>(depth) } << 32) |
         (u64{ static_cast<std::uint8_t>(flag) } << 48);
}

int TranspositionTable::unpacked_score(const u64 data) {
  return static_cast<std::int32_t>(static_cast<std::uint32_t>(data));
}

int TranspositionTable::unpacked_depth(const u64 data) {
  return static_cast<std::int16_t>(static_cast<std::uint16_t>(data >> 32));
}

HashFlag TranspositionTable::unpacked_flag(const u64 data) {
  return static_cast<HashFlag>(static_cast<std::uint8_t>(data >> 48));
}

int TranspositionTable::read(const u64 hash_key, const int alpha, const int beta, const int depth, const int ply) const {
  if(table_.empty())
    return no_entry;

  const Entry& entry = table_[hash_key % table_.size()];
  const u64 data = entry.data.load(std::memory_order_relaxed);

  if((entry.checked_key.load(std::memory_order_relaxed) ^ data) != hash_key || unpacked_depth(data) < depth)
    return no_entry;

  int score = unpacked_score(data);
  const HashFlag flag = unpacked_flag(data);
  if(score < -Scores::mate_score)
    score += ply;
  if(score > Scores::mate_score)
    score -= ply;

  if(flag == HashFlag::exact)
    return score;
  if(flag == HashFlag::alpha && score <= alpha)
    return alpha;
  if(flag == HashFlag::beta && score >= beta)
    return beta;

  return no_entry;
}

void TranspositionTable::write(const u64 hash_key, int score, const int depth, const HashFlag flag, const int ply) {
  if(table_.empty())
    return;

  Entry& entry = table_[hash_key % table_.size()];

  if(score < -Scores::mate_score)
    score -= ply;
  if(score > Scores::mate_score)
    score += ply;

  const u64 data = pack(score, depth, flag);
  entry.checked_key.store(hash_key ^ data, std::memory_order_relaxed);
  entry.data.store(data, std::memory_order_relaxed);
}

} // namespace maharajah
