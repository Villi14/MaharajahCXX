#pragma once

namespace maharajah {

enum class EvalMode : int { classic, nnue };

// Strength-related search parameters. `difficulty 1..5` is the level shown to players;
// `skill 1..10` is the lower layer it maps onto.
struct SearchConfig {
  int ui_difficulty{ };
  int skill_level{ };
  int max_depth_cap{ };
  // below this many plies of game history a weaker skill may pick a near-best move
  int opening_variety_plies{ };
  int weak_move_margin_cp{ };
  int weak_move_candidates{ };
  int reverse_futility_margin_per_depth{ };
  int futility_margin_per_depth{ };
  int late_move_pruning_base{ };
  int late_move_pruning_scale{ };
  int history_bonus_scale{ };
  // quiescence skips a capture when see < -margin and stand_pat + see < alpha
  int quiescence_see_prune_margin{ };
  // a repeating best move gives way to a non-repeating one scoring within this margin
  int anti_repeat_margin{ 150 };
  // a weak-skill candidate must verify within this margin of the best move
  int tactical_margin{ 120 };

  static constexpr int min_skill{ 1 };
  static constexpr int max_skill{ 10 };
  static constexpr int min_difficulty{ 1 };
  static constexpr int max_difficulty{ 5 };

  [[nodiscard]] static SearchConfig for_skill(int skill_level);
  [[nodiscard]] static SearchConfig for_difficulty(int difficulty_level);
};

// NNUE by default (every Engine loads the built-in network); the classic
// evaluation's weights are in Evaluation.h.
struct EvalConfig {
  EvalMode eval_mode{ EvalMode::nnue };
};

} // namespace maharajah
