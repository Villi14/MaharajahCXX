// Texel tuning of the classic evaluation weights (Evaluation::weights).
//
//   maharajah_texel gen OUT [games] [depth] [threads] [seed] [custom_share]
//     Self-play at a fixed depth from random openings (the start position plus random
//     plies, or a generated custom army plus random plies). Writes "FEN;result" per
//     position after the opening, result 1 / 0.5 / 0 from white's point of view.
//   maharajah_texel tune DATA [epochs] [threads] [out]
//     Keeps the quiet positions (quiescence search = static evaluation, not in check),
//     fits the sigmoid scale K, then fits both phases' weights to the results by Adam
//     on the mean squared error. Writes the new `weights` initializer of Evaluation.h
//     to `out` (default texel_weights.txt).
#include "../headers/Bitboard.h"
#include "../headers/Constants.h"
#include "../headers/CustomSetup.h"
#include "../headers/Engine.h"
#include "../headers/Evaluation.h"
#include "../headers/Evaluator.h"
#include "../headers/Search.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

using namespace maharajah;

namespace {

// ---------------------------------------------------------------------------
// gen

struct GenOptions {
  std::string out;
  int games{ 1000 };
  int depth{ 8 };
  int threads{ 1 };
  unsigned seed{ 1 };
  double custom_share{ 0.3 };
};

constexpr int max_plies{ 400 };
// a side is adjudicated the winner after this many plies at >= win_score for it
constexpr int win_score{ 1000 };
constexpr int win_plies{ 6 };
// a draw after this many plies within +-draw_score, from draw_from_ply on
constexpr int draw_score{ 15 };
constexpr int draw_plies{ 12 };
constexpr int draw_from_ply{ 80 };

std::mutex rand_mutex;
std::mutex out_mutex;

std::vector<int> legal_moves(const Board& board) {
  MoveList moves;
  board.generate_moves(moves);
  std::vector<int> legal;
  for(std::size_t i{ }; i < moves.size(); ++i) {
    Board copy = board;
    if(copy.make_move(moves[i], TypeMove::all_moves))
      legal.push_back(moves[i]);
  }
  return legal;
}

// Plays one game; returns false if the opening ended the game already.
bool play_game(Engine& engine, std::mt19937& rng, const GenOptions& options, std::FILE* out, std::atomic<long>& positions) {
  std::string fen{ Fen::start_position };
  int opening_plies = std::uniform_int_distribution(6, 10)(rng);

  if(std::uniform_real_distribution(0.0, 1.0)(rng) < options.custom_share) {
    const std::lock_guard lock(rand_mutex);
    Board board;
    if(!generate_custom_position(board, -1, static_cast<unsigned>(rng())))
      return false;
    fen = custom_position_fen(board);
    opening_plies = std::uniform_int_distribution(0, 3)(rng);
  }

  engine.set_position(fen);
  engine.transposition_table.clear();

  for(int ply{ }; ply < opening_plies; ++ply) {
    const std::vector<int> legal = legal_moves(engine.board);
    if(legal.empty())
      return false;
    const int move = legal[std::uniform_int_distribution<std::size_t>(0, legal.size() - 1)(rng)];
    engine.apply_move(Board::move_to_string(move));
  }

  std::vector<std::string> fens;
  double result = 0.5;
  int white_wins{ }, black_wins{ }, quiet{ };

  for(int ply{ }; ply < max_plies; ++ply) {
    Board& board = engine.board;
    if(!board.has_legal_move()) {
      if(board.in_check())
        result = board.state.side == white ? 0.0 : 1.0;
      break;
    }
    if(Search(engine).is_draw())
      break;

    const SearchResult searched = run_search(engine, options.depth);
    if(!searched.best_move)
      break;

    const int score = board.state.side == white ? searched.score : -searched.score;
    white_wins = score >= win_score ? white_wins + 1 : 0;
    black_wins = score <= -win_score ? black_wins + 1 : 0;
    quiet = ply + opening_plies >= draw_from_ply && std::abs(score) <= draw_score ? quiet + 1 : 0;

    if(!board.in_check())
      fens.push_back(board.to_fen());

    if(white_wins >= win_plies) {
      result = 1.0;
      break;
    }
    if(black_wins >= win_plies) {
      result = 0.0;
      break;
    }
    if(quiet >= draw_plies)
      break;

    if(!engine.apply_move(Board::move_to_string(searched.best_move)))
      break;
  }

  const std::lock_guard lock(out_mutex);
  for(const std::string& position : fens)
    std::fprintf(out, "%s;%.1f\n", position.c_str(), result);
  std::fflush(out);
  positions += static_cast<long>(fens.size());
  return true;
}

int run_gen(const GenOptions& options) {
  std::FILE* out = std::fopen(options.out.c_str(), "w");
  if(!out) {
    std::cerr << "cannot write " << options.out << '\n';
    return 1;
  }

  std::atomic<int> next_game{ };
  std::atomic<long> positions{ };
  std::vector<std::thread> threads;

  for(int index{ }; index < options.threads; ++index) {
    threads.emplace_back([&, index] {
      Engine engine;
      engine.transposition_table.resize(16);
      std::mt19937 rng(options.seed * 7919u + static_cast<unsigned>(index));
      int game;
      while((game = next_game++) < options.games) {
        play_game(engine, rng, options, out, positions);
        if(game % 100 == 0)
          std::cerr << "game " << game << ", positions " << positions << '\n';
      }
    });
  }
  for(std::thread& thread : threads)
    thread.join();

  std::fclose(out);
  std::cerr << "done: " << options.games << " games, " << positions << " positions\n";
  return 0;
}

// ---------------------------------------------------------------------------
// tune

constexpr int weight_count{ eval_weight_count };

struct Coefficient {
  std::uint16_t index;
  std::int16_t value;
};

struct Position {
  std::uint32_t first;
  std::uint16_t count;
  std::int16_t phase;
  std::int32_t fixed;
  std::int8_t side_sign;
  float result;
};

struct Dataset {
  std::vector<Position> positions;
  std::vector<Coefficient> coefficients;
};

using Weights = std::array<std::vector<double>, 2>;

double position_eval(const Dataset& data, const Position& position, const Weights& weights) {
  double opening_score = position.fixed, endgame_score = position.fixed;
  for(std::uint32_t i{ position.first }; i < position.first + position.count; ++i) {
    const Coefficient& c = data.coefficients[i];
    opening_score += c.value * weights[opening][c.index];
    endgame_score += c.value * weights[endgame][c.index];
  }
  const double phase = position.phase;
  return (opening_score * phase + endgame_score * (Evaluator::phase_range - phase)) / Evaluator::phase_range + position.side_sign * Evaluation::tempo_bonus;
}

double sigmoid(const double k, const double eval) {
  return 1.0 / (1.0 + std::exp(-k * eval));
}

// k is K * ln(10) / 400, so sigmoid(k, eval) = 1 / (1 + 10^(-K * eval / 400))
double mean_error(const Dataset& data, const Weights& weights, const double k, const int threads) {
  std::vector<double> sums(static_cast<std::size_t>(threads));
  std::vector<std::thread> workers;
  const std::size_t n = data.positions.size();
  for(int t{ }; t < threads; ++t) {
    workers.emplace_back([&, t] {
      double sum{ };
      for(std::size_t i = n * t / threads; i < n * (t + 1) / threads; ++i) {
        const Position& position = data.positions[i];
        const double error = position.result - sigmoid(k, position_eval(data, position, weights));
        sum += error * error;
      }
      sums[static_cast<std::size_t>(t)] = sum;
    });
  }
  for(std::thread& worker : workers)
    worker.join();
  double total{ };
  for(const double sum : sums)
    total += sum;
  return total / static_cast<double>(n);
}

void gradient(const Dataset& data, const Weights& weights, const double k, const int threads, Weights& out) {
  std::vector<Weights> partial(static_cast<std::size_t>(threads));
  std::vector<std::thread> workers;
  const std::size_t n = data.positions.size();
  for(int t{ }; t < threads; ++t) {
    workers.emplace_back([&, t] {
      Weights& g = partial[static_cast<std::size_t>(t)];
      g[opening].assign(weight_count, 0.0);
      g[endgame].assign(weight_count, 0.0);
      for(std::size_t i = n * t / threads; i < n * (t + 1) / threads; ++i) {
        const Position& position = data.positions[i];
        const double s = sigmoid(k, position_eval(data, position, weights));
        const double d = -2.0 * (position.result - s) * s * (1.0 - s) * k;
        const double opening_share = static_cast<double>(position.phase) / Evaluator::phase_range;
        for(std::uint32_t j{ position.first }; j < position.first + position.count; ++j) {
          const Coefficient& c = data.coefficients[j];
          g[opening][c.index] += d * c.value * opening_share;
          g[endgame][c.index] += d * c.value * (1.0 - opening_share);
        }
      }
    });
  }
  for(std::thread& worker : workers)
    worker.join();
  for(const int phase : { opening, endgame }) {
    out[phase].assign(weight_count, 0.0);
    for(const Weights& g : partial) {
      for(int i{ }; i < weight_count; ++i)
        out[phase][i] += g[phase][i] / static_cast<double>(n);
    }
  }
}

Dataset load(const std::string& path, const int threads) {
  std::ifstream in(path);
  std::vector<std::string> lines;
  for(std::string line; std::getline(in, line);) {
    if(!line.empty())
      lines.push_back(line);
  }
  std::cerr << lines.size() << " positions read\n";

  std::vector<Dataset> parts(static_cast<std::size_t>(threads));
  std::atomic<long> skipped_check{ }, skipped_noisy{ }, skipped_large{ }, mismatches{ };
  std::vector<std::thread> workers;
  for(int t{ }; t < threads; ++t) {
    workers.emplace_back([&, t] {
      Engine engine;
      engine.transposition_table.resize(1);
      Dataset& part = parts[static_cast<std::size_t>(t)];
      EvalTrace trace;
      for(std::size_t i = lines.size() * t / threads; i < lines.size() * (t + 1) / threads; ++i) {
        const std::string& line = lines[i];
        const std::size_t separator = line.rfind(';');
        if(separator == std::string::npos || !engine.set_position(line.substr(0, separator)))
          continue;
        Board& board = engine.board;
        if(board.in_check()) {
          ++skipped_check;
          continue;
        }
        const int static_eval = Evaluator::evaluate(board.state);
        if(Search(engine).quiescence(-Scores::infinity, Scores::infinity) != static_eval) {
          ++skipped_noisy;
          continue;
        }
        if(std::abs(static_eval) > 2500) {
          ++skipped_large;
          continue;
        }

        const int white_eval = Evaluator::evaluate_white(board.state, trace);
        Position position{ };
        position.first = static_cast<std::uint32_t>(part.coefficients.size());
        position.phase = static_cast<std::int16_t>(trace.phase);
        position.fixed = trace.fixed;
        position.side_sign = board.state.side == white ? 1 : -1;
        position.result = std::stof(line.substr(separator + 1));
        for(int index{ }; index < weight_count; ++index) {
          if(trace.coefficients[static_cast<std::size_t>(index)] != 0)
            part.coefficients.push_back({ static_cast<std::uint16_t>(index), static_cast<std::int16_t>(trace.coefficients[static_cast<std::size_t>(index)]) });
        }
        position.count = static_cast<std::uint16_t>(part.coefficients.size() - position.first);

        // the traced evaluation must reproduce the engine's (up to integer rounding)
        static const Weights defaults = [] {
          Weights w;
          for(const int phase : { opening, endgame }) {
            const int* base = reinterpret_cast<const int*>(&Evaluation::weights[static_cast<std::size_t>(phase)]);
            w[phase].assign(base, base + weight_count);
          }
          return w;
        }();
        const double traced = position_eval(part, position, defaults);
        const int engine_white = board.state.side == white ? static_eval : -static_eval;
        if(std::abs(traced - engine_white) > 1.0 || white_eval + position.side_sign * Evaluation::tempo_bonus != engine_white)
          ++mismatches;

        part.positions.push_back(position);
      }
    });
  }
  for(std::thread& worker : workers)
    worker.join();

  Dataset data;
  for(Dataset& part : parts) {
    const auto offset = static_cast<std::uint32_t>(data.coefficients.size());
    for(Position position : part.positions) {
      position.first += offset;
      data.positions.push_back(position);
    }
    data.coefficients.insert(data.coefficients.end(), part.coefficients.begin(), part.coefficients.end());
  }
  std::cerr << data.positions.size() << " quiet positions (skipped: " << skipped_check << " in check, " << skipped_noisy << " noisy, " << skipped_large
            << " |eval| > 2500), " << data.coefficients.size() << " coefficients, " << mismatches << " trace mismatches\n";
  return data;
}

double fit_k(const Dataset& data, const Weights& weights, const int threads) {
  double low = 0.1, high = 4.0;
  const auto error_at = [&](const double scale) { return mean_error(data, weights, scale * std::log(10.0) / 400.0, threads); };
  for(int step{ }; step < 40; ++step) {
    const double a = low + (high - low) * 0.382, b = low + (high - low) * 0.618;
    if(error_at(a) < error_at(b))
      high = b;
    else
      low = a;
  }
  return (low + high) / 2;
}

// --- output in the layout of Evaluation.h

const char* const kind_names[]{ "pawn", "knight", "bishop", "rook", "queen", "king" };

void write_weights(std::ostream& out, const Weights& weights) {
  for(const int phase : { opening, endgame }) {
    std::vector<int> w(weight_count);
    for(int i{ }; i < weight_count; ++i)
      w[static_cast<std::size_t>(i)] = static_cast<int>(std::lround(weights[phase][static_cast<std::size_t>(i)]));
    EvalWeights e;
    std::copy(w.begin(), w.end(), reinterpret_cast<int*>(&e));

    const auto list = [](const auto& values) {
      std::string text;
      for(std::size_t i{ }; i < values.size(); ++i)
        text += (i ? ", " : "") + std::to_string(values[i]);
      return text;
    };

    out << "  // " << (phase == opening ? "opening" : "endgame") << "\n  EvalWeights{\n";
    out << "    .material{ " << list(e.material) << " },\n";
    out << "    .positional{ {\n";
    for(int kind{ }; kind < 6; ++kind) {
      out << "      // " << kind_names[kind] << "\n      {\n";
      for(int row{ }; row < 8; ++row) {
        out << "      ";
        for(int file{ }; file < 8; ++file) {
          char cell[16];
          std::snprintf(cell, sizeof cell, file ? ",%5d" : "%6d", e.positional[static_cast<std::size_t>(kind)][static_cast<std::size_t>(row * 8 + file)]);
          out << cell;
        }
        out << (row == 7 ? "\n" : ",\n");
      }
      out << "      },\n";
    }
    out << "    } },\n";
    const std::pair<const char*, int> scalars[]{
      { "double_pawn", e.double_pawn },
      { "isolated_pawn", e.isolated_pawn },
      { "rook_semi_open_file", e.rook_semi_open_file },
      { "rook_open_file", e.rook_open_file },
      { "king_semi_open_file", e.king_semi_open_file },
      { "king_open_file", e.king_open_file },
      { "king_shield", e.king_shield },
      { "knight_mobility", e.knight_mobility },
      { "bishop_mobility", e.bishop_mobility },
      { "rook_mobility", e.rook_mobility },
      { "queen_mobility", e.queen_mobility },
      { "archbishop_mobility", e.archbishop_mobility },
      { "chancellor_mobility", e.chancellor_mobility },
      { "amazon_mobility", e.amazon_mobility },
      { "bishop_pair", e.bishop_pair },
    };
    for(const auto& [name, value] : scalars)
      out << "    ." << name << "{ " << value << " },\n";
    out << "    .passed_pawn{ " << list(e.passed_pawn) << " },\n";
    out << "  },\n";
  }
}

int run_tune(const std::string& path, const int epochs, const int threads, const std::string& out_path) {
  const Dataset data = load(path, threads);
  if(data.positions.empty())
    return 1;

  Weights weights;
  for(const int phase : { opening, endgame }) {
    const int* base = reinterpret_cast<const int*>(&Evaluation::weights[static_cast<std::size_t>(phase)]);
    weights[phase].assign(base, base + weight_count);
  }

  const double scale = fit_k(data, weights, threads);
  const double k = scale * std::log(10.0) / 400.0;
  std::cerr << "K = " << scale << ", error " << mean_error(data, weights, k, threads) << '\n';

  // Adam
  constexpr double rate{ 1.0 }, beta1{ 0.9 }, beta2{ 0.999 }, epsilon{ 1e-8 };
  Weights m, v, g;
  for(const int phase : { opening, endgame }) {
    m[phase].assign(weight_count, 0.0);
    v[phase].assign(weight_count, 0.0);
  }
  for(int epoch{ 1 }; epoch <= epochs; ++epoch) {
    gradient(data, weights, k, threads, g);
    for(const int phase : { opening, endgame }) {
      for(int i{ }; i < weight_count; ++i) {
        m[phase][i] = beta1 * m[phase][i] + (1 - beta1) * g[phase][i];
        v[phase][i] = beta2 * v[phase][i] + (1 - beta2) * g[phase][i] * g[phase][i];
        const double m_hat = m[phase][i] / (1 - std::pow(beta1, epoch));
        const double v_hat = v[phase][i] / (1 - std::pow(beta2, epoch));
        weights[phase][i] -= rate * m_hat / (std::sqrt(v_hat) + epsilon);
      }
    }
    if(epoch % 100 == 0 || epoch == epochs)
      std::cerr << "epoch " << epoch << ", error " << mean_error(data, weights, k, threads) << '\n';
  }

  std::ofstream out(out_path);
  write_weights(out, weights);
  std::cerr << "weights written to " << out_path << '\n';
  return 0;
}

} // namespace

int main(const int argc, char** argv) {
  const std::string command = argc > 1 ? argv[1] : "";
  const auto arg = [&](const int index, const auto fallback) {
    using T = std::remove_cv_t<decltype(fallback)>;
    if(argc <= index)
      return fallback;
    if constexpr(std::is_same_v<T, double>)
      return std::stod(argv[index]);
    else
      return static_cast<T>(std::stol(argv[index]));
  };

  if(command == "gen" && argc > 2) {
    GenOptions options;
    options.out = argv[2];
    options.games = arg(3, options.games);
    options.depth = arg(4, options.depth);
    options.threads = arg(5, options.threads);
    options.seed = arg(6, options.seed);
    options.custom_share = arg(7, options.custom_share);
    return run_gen(options);
  }

  if(command == "tune" && argc > 2)
    return run_tune(argv[2], arg(3, 1000), arg(4, 1), argc > 5 ? argv[5] : "texel_weights.txt");

  std::cerr << "usage: " << argv[0] << " gen OUT [games] [depth] [threads] [seed] [custom_share]\n"
            << "       " << argv[0] << " tune DATA [epochs] [threads] [out]\n";
  return 2;
}
