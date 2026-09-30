#include "../headers/Transposition.h"

#include <algorithm>

namespace maharajah {

void TranspositionTable::resize(const int mb) {
  const std::size_t bytes = std::size_t{ 0x100000 } * static_cast<std::size_t>(std::clamp(mb, min_mb, max_mb));
  table_.assign(bytes / sizeof(Entry), Entry{ });
}

void TranspositionTable::clear() {
  std::fill(table_.begin(), table_.end(), Entry{ });
}

int TranspositionTable::read(const u64 hash_key, const int alpha, const int beta, const int depth, const int ply) const {
  if(table_.empty())
    return no_entry;

  const Entry& entry = table_[hash_key % table_.size()];

  if(entry.hash_key != hash_key || entry.depth < depth)
    return no_entry;

  int score = entry.score;
  if(score < -Scores::mate_score)
    score += ply;
  if(score > Scores::mate_score)
    score -= ply;

  if(entry.flag == HashFlag::exact)
    return score;
  if(entry.flag == HashFlag::alpha && score <= alpha)
    return alpha;
  if(entry.flag == HashFlag::beta && score >= beta)
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

  entry = Entry{ hash_key, depth, flag, score };
}

} // namespace maharajah
