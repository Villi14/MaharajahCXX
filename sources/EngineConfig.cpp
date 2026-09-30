#include "../headers/EngineConfig.h"

#include <algorithm>

namespace maharajah {

namespace {

struct SkillProfile {
  int max_depth_cap;
  int opening_variety_plies;
  int weak_move_margin_cp;
  int weak_move_candidates;
  int reverse_futility_margin_per_depth;
  int futility_margin_per_depth;
  int late_move_pruning_base;
  int late_move_pruning_scale;
  int history_bonus_scale;
};

constexpr std::array<int, 5> ui_to_skill_level{ 2, 4, 6, 8, 10 };

// clang-format off
constexpr std::array<SkillProfile, 10> skill_profiles{ {
  { 1, 14, 240, 5, 80, 160, 6, 1, 1 },
  { 1, 14, 220, 5, 80, 150, 6, 1, 1 },
  { 2, 12, 205, 5, 85, 145, 7, 1, 1 },
  { 2, 12, 190, 4, 85, 140, 7, 1, 1 },
  { 3, 10, 170, 4, 90, 130, 8, 2, 1 },
  { 4, 10, 150, 4, 90, 125, 8, 2, 1 },
  { 5,  8, 120, 3, 95, 120, 8, 2, 1 },
  { 6,  6,  90, 3, 95, 120, 8, 2, 1 },
  { 8,  4,  50, 2, 100, 120, 8, 2, 1 },
  { Limits::max_ply, 0, 0, 1, 90, 120, 8, 2, 2 },
} };
// clang-format on

} // namespace

SearchConfig SearchConfig::for_skill(const int skill_level) {
  const int clamped = std::clamp(skill_level, min_skill, max_skill);
  const SkillProfile& profile = skill_profiles[clamped - 1];

  SearchConfig config{ };
  config.skill_level = clamped;
  config.max_depth_cap = profile.max_depth_cap;
  config.opening_variety_plies = profile.opening_variety_plies;
  config.weak_move_margin_cp = profile.weak_move_margin_cp;
  config.weak_move_candidates = profile.weak_move_candidates;
  config.reverse_futility_margin_per_depth = profile.reverse_futility_margin_per_depth;
  config.futility_margin_per_depth = profile.futility_margin_per_depth;
  config.late_move_pruning_base = profile.late_move_pruning_base;
  config.late_move_pruning_scale = profile.late_move_pruning_scale;
  config.history_bonus_scale = profile.history_bonus_scale;
  return config;
}

SearchConfig SearchConfig::for_difficulty(const int difficulty_level) {
  const int clamped = std::clamp(difficulty_level, min_difficulty, max_difficulty);
  SearchConfig config = for_skill(ui_to_skill_level[clamped - 1]);
  config.ui_difficulty = clamped;
  return config;
}

} // namespace maharajah
