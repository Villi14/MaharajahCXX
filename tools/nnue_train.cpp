// Trains the NNUE network (Nnue.h) on self-play data from `maharajah_texel gen`.
//
//   maharajah_nnue_train DATA[,DATA...] OUT.nnue [epochs] [threads] [lambda] [seed] [hidden]
//     DATA lines are "FEN;score;result" (score and result from white's point of view).
//     The target blends the search score and the result:
//     lambda * sigmoid(score / 400) + (1 - lambda) * result, fitted by AdamW on the
//     mean squared error of sigmoid(network output). 2 % of the positions are held
//     out for validation. The float network is quantized as Nnue.h reads it and
//     checked against the engine's evaluation of the held-out positions.
//     Writes OUT.nnue after every epoch and the float network to OUT.nnue.pt at the end.
//     Built only when CMake finds libtorch (CMAKE_PREFIX_PATH=~/libtorch).
#include "../headers/Board.h"
#include "../headers/Nnue.h"

#include <torch/torch.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <numbers>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace maharajah;
namespace F = torch::nn::functional;

namespace {

constexpr int batch_size{ 16384 };
// float weights stay where the int16 quantization can hold them; an output weight
// times qa must fit int16 (NnueSimd.h screlu_dot)
constexpr double feature_clip{ 1.98 };
constexpr double output_clip{ 127.0 / NnueArch::qb };
constexpr double eval_scale{ NnueArch::scale };

// Positions with their input features: for position i, features
// [first[i], first[i] + count[i]) of `side_to_move` and of `other` are the active
// inputs from the side to move's and the other side's point of view.
struct Dataset {
  std::vector<std::int16_t> side_to_move, other;
  std::vector<std::uint32_t> first;
  std::vector<std::uint8_t> count;
  // side to move's point of view
  std::vector<float> score, result;
  std::vector<std::string> fens;

  [[nodiscard]] std::size_t size() const {
    return first.size();
  }
};

void append(Dataset& data, const BoardState& state, const float white_score, const float white_result, const std::string& fen, const bool keep_fen) {
  const Colors side = state.side;
  data.first.push_back(static_cast<std::uint32_t>(data.side_to_move.size()));
  int count{ };
  for(int piece{ }; piece < PieceCount::all; ++piece) {
    for(u64 bitboard = state.bitboards[piece]; bitboard; bitboard &= bitboard - 1) {
      const int square = std::countr_zero(bitboard);
      data.side_to_move.push_back(static_cast<std::int16_t>(NnueArch::feature(side, piece, square)));
      data.other.push_back(static_cast<std::int16_t>(NnueArch::feature(opponent(side), piece, square)));
      ++count;
    }
  }
  data.count.push_back(static_cast<std::uint8_t>(count));
  data.score.push_back(side == white ? white_score : -white_score);
  data.result.push_back(side == white ? white_result : 1.0f - white_result);
  if(keep_fen)
    data.fens.push_back(fen);
}

// Reads every file of the comma-separated list; every 50th position goes to `validation`.
void load(const std::string& paths, const int threads, Dataset& training, Dataset& validation) {
  std::vector<std::string> lines;
  std::stringstream list(paths);
  for(std::string path; std::getline(list, path, ',');) {
    std::ifstream in(path);
    if(!in)
      std::cerr << "cannot read " << path << '\n';
    for(std::string line; std::getline(in, line);) {
      if(!line.empty())
        lines.push_back(std::move(line));
    }
  }
  std::cerr << lines.size() << " lines read\n";

  std::vector<Dataset> train_parts(static_cast<std::size_t>(threads)), valid_parts(static_cast<std::size_t>(threads));
  std::atomic<long> rejected{ };
  std::vector<std::thread> workers;
  for(int t{ }; t < threads; ++t) {
    workers.emplace_back([&, t] {
      Board board;
      for(std::size_t i = lines.size() * t / threads; i < lines.size() * (t + 1) / threads; ++i) {
        const std::string& line = lines[i];
        const std::size_t first = line.find(';'), last = line.rfind(';');
        if(first == std::string::npos || first == last) {
          ++rejected;
          continue;
        }
        try {
          board.parse_fen(std::string_view(line).substr(0, first));
        } catch(const std::exception&) {
          ++rejected;
          continue;
        }
        const float score = std::stof(line.substr(first + 1, last - first - 1));
        const float result = std::stof(line.substr(last + 1));
        const bool held_out = i % 50 == 0;
        append(held_out ? valid_parts[static_cast<std::size_t>(t)] : train_parts[static_cast<std::size_t>(t)], board.state, score, result, line.substr(0, first), held_out);
      }
    });
  }
  for(std::thread& worker : workers)
    worker.join();

  const auto merge = [](std::vector<Dataset>& parts, Dataset& out) {
    for(Dataset& part : parts) {
      const auto offset = static_cast<std::uint32_t>(out.side_to_move.size());
      for(const std::uint32_t first : part.first)
        out.first.push_back(first + offset);
      out.side_to_move.insert(out.side_to_move.end(), part.side_to_move.begin(), part.side_to_move.end());
      out.other.insert(out.other.end(), part.other.begin(), part.other.end());
      out.count.insert(out.count.end(), part.count.begin(), part.count.end());
      out.score.insert(out.score.end(), part.score.begin(), part.score.end());
      out.result.insert(out.result.end(), part.result.begin(), part.result.end());
      out.fens.insert(out.fens.end(), part.fens.begin(), part.fens.end());
      part = Dataset{ };
    }
  };
  merge(train_parts, training);
  merge(valid_parts, validation);
  std::cerr << training.size() << " training, " << validation.size() << " validation positions, " << rejected << " lines rejected\n";
}

struct Batch {
  torch::Tensor side_to_move, side_to_move_offsets, other, other_offsets, target;
};

Batch make_batch(const Dataset& data, const std::vector<std::uint32_t>& order, const std::size_t begin, const std::size_t end, const float lambda) {
  std::vector<std::int64_t> stm, nstm, offsets;
  std::vector<float> target;
  stm.reserve((end - begin) * 34);
  nstm.reserve((end - begin) * 34);
  for(std::size_t i{ begin }; i < end; ++i) {
    const std::uint32_t index = order[i];
    offsets.push_back(static_cast<std::int64_t>(stm.size()));
    for(std::uint32_t j{ data.first[index] }; j < data.first[index] + data.count[index]; ++j) {
      stm.push_back(data.side_to_move[j]);
      nstm.push_back(data.other[j]);
    }
    const float score_target = 1.0f / (1.0f + std::exp(-data.score[index] / static_cast<float>(eval_scale)));
    target.push_back(lambda * score_target + (1.0f - lambda) * data.result[index]);
  }
  const auto tensor = [](const auto& values, const torch::Dtype type) {
    return torch::from_blob(const_cast<void*>(static_cast<const void*>(values.data())), { static_cast<std::int64_t>(values.size()) }, type).clone();
  };
  const torch::Tensor offset_tensor = tensor(offsets, torch::kInt64);
  return { tensor(stm, torch::kInt64), offset_tensor, tensor(nstm, torch::kInt64), offset_tensor, tensor(target, torch::kFloat32).unsqueeze(1) };
}

struct NetworkImpl : torch::nn::Module {
  explicit NetworkImpl(const int hidden)
      : hidden(hidden) {
    feature_weights = register_parameter("feature_weights", torch::randn({ NnueArch::inputs, hidden }) * 0.05);
    feature_bias = register_parameter("feature_bias", torch::zeros({ hidden }));
    output_weights = register_parameter("output_weights", (torch::rand({ 2 * hidden, 1 }) * 2 - 1) / std::sqrt(2.0 * hidden));
    output_bias = register_parameter("output_bias", torch::zeros({ 1 }));
  }

  // network output (eval / scale)
  torch::Tensor forward(const Batch& batch) {
    const auto bag = [&](const torch::Tensor& indices, const torch::Tensor& offsets) {
      const torch::Tensor accumulator = F::embedding_bag(indices, feature_weights, F::EmbeddingBagFuncOptions().offsets(offsets).mode(torch::kSum)) + feature_bias;
      return torch::clamp(accumulator, 0.0, 1.0).square();
    };
    const torch::Tensor activated = torch::cat({ bag(batch.side_to_move, batch.side_to_move_offsets), bag(batch.other, batch.other_offsets) }, 1);
    return activated.matmul(output_weights) + output_bias;
  }

  void clip() {
    torch::NoGradGuard no_grad;
    feature_weights.clamp_(-feature_clip, feature_clip);
    output_weights.clamp_(-output_clip, output_clip);
  }

  int hidden;
  torch::Tensor feature_weights, feature_bias, output_weights, output_bias;
};
TORCH_MODULE(Network);

double validation_loss(Network& network, const Dataset& data, const float lambda) {
  torch::NoGradGuard no_grad;
  std::vector<std::uint32_t> order(data.size());
  std::iota(order.begin(), order.end(), 0u);
  double sum{ };
  for(std::size_t begin{ }; begin < data.size(); begin += batch_size) {
    const std::size_t end = std::min(data.size(), begin + batch_size);
    const Batch batch = make_batch(data, order, begin, end, lambda);
    sum += (torch::sigmoid(network->forward(batch)) - batch.target).square().sum().item<double>();
  }
  return sum / static_cast<double>(data.size());
}

void put_u32(std::ofstream& out, const std::uint32_t value) {
  for(int shift{ }; shift < 32; shift += 8)
    out.put(static_cast<char>(value >> shift));
}

void put_i16(std::ofstream& out, const torch::Tensor& values, const double factor) {
  const torch::Tensor quantized = torch::round(values.contiguous().view(-1) * factor).clamp(-32767, 32767).to(torch::kInt32);
  const auto accessor = quantized.accessor<int, 1>();
  for(std::int64_t i{ }; i < accessor.size(0); ++i) {
    const auto bits = static_cast<std::uint16_t>(static_cast<std::int16_t>(accessor[i]));
    out.put(static_cast<char>(bits));
    out.put(static_cast<char>(bits >> 8));
  }
}

void write_network(Network& network, const std::string& path) {
  torch::NoGradGuard no_grad;
  std::ofstream out(path, std::ios::binary);
  out.write("MHNN", 4);
  put_u32(out, NnueArch::version);
  put_u32(out, NnueArch::inputs);
  put_u32(out, static_cast<std::uint32_t>(network->hidden));
  put_i16(out, network->feature_weights, NnueArch::qa);
  put_i16(out, network->feature_bias, NnueArch::qa);
  put_i16(out, network->output_weights, NnueArch::qb);
  put_u32(out, static_cast<std::uint32_t>(static_cast<std::int32_t>(std::lround(network->output_bias.item<double>() * NnueArch::qa * NnueArch::qb))));
}

// the quantized network in the engine against the float one on the held-out positions
void check_quantization(Network& network, const Dataset& validation, const std::string& path) {
  Nnue nnue;
  if(!nnue.load_weights(path.c_str())) {
    std::cerr << "the engine rejects " << path << '\n';
    return;
  }
  torch::NoGradGuard no_grad;
  const std::size_t n = std::min<std::size_t>(validation.size(), 20000);
  std::vector<std::uint32_t> order(n);
  std::iota(order.begin(), order.end(), 0u);
  const torch::Tensor float_eval = network->forward(make_batch(validation, order, 0, n, 1.0f)) * eval_scale;
  const auto accessor = float_eval.accessor<float, 2>();
  double abs_error{ }, max_error{ }, abs_eval{ };
  Board board;
  for(std::size_t i{ }; i < n; ++i) {
    board.parse_fen(validation.fens[i]);
    const double error = std::abs(nnue.evaluate(board.state) - accessor[static_cast<std::int64_t>(i)][0]);
    abs_error += error;
    max_error = std::max(max_error, error);
    abs_eval += std::abs(accessor[static_cast<std::int64_t>(i)][0]);
  }
  std::cerr << "quantized vs float on " << n << " positions: mean |diff| " << abs_error / static_cast<double>(n) << " cp, max " << max_error
            << " cp (mean |eval| " << abs_eval / static_cast<double>(n) << " cp)\n";
}

int run(const std::string& data_paths, const std::string& out_path, const int epochs, const int threads, const float lambda, const unsigned seed, const int hidden) {
  torch::set_num_threads(threads);
  torch::manual_seed(seed);

  Dataset training, validation;
  load(data_paths, threads, training, validation);
  if(training.size() < batch_size || validation.size() == 0) {
    std::cerr << "not enough positions\n";
    return 1;
  }

  Network network(hidden);
  constexpr double start_rate{ 1e-3 };
  torch::optim::AdamW optimizer(network->parameters(), torch::optim::AdamWOptions(start_rate).weight_decay(0.0));

  std::mt19937 rng(seed);
  std::vector<std::uint32_t> order(training.size());
  std::iota(order.begin(), order.end(), 0u);
  std::cerr << "validation loss " << validation_loss(network, validation, lambda) << " before training\n";

  for(int epoch{ 1 }; epoch <= epochs; ++epoch) {
    // the rate drops tenfold over the last 30 % of the epochs, cosine-shaped
    const double progress = std::clamp((static_cast<double>(epoch - 1) / epochs - 0.7) / 0.3, 0.0, 1.0);
    const double rate = start_rate * (0.1 + 0.9 * 0.5 * (1 + std::cos(std::numbers::pi * progress)));
    for(auto& group : optimizer.param_groups())
      static_cast<torch::optim::AdamWOptions&>(group.options()).lr(rate);

    std::shuffle(order.begin(), order.end(), rng);
    double sum{ };
    const auto start = std::chrono::steady_clock::now();
    for(std::size_t begin{ }; begin + batch_size <= training.size(); begin += batch_size) {
      const Batch batch = make_batch(training, order, begin, begin + batch_size, lambda);
      optimizer.zero_grad();
      const torch::Tensor loss = (torch::sigmoid(network->forward(batch)) - batch.target).square().mean();
      loss.backward();
      optimizer.step();
      network->clip();
      sum += loss.item<double>() * batch_size;
    }
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::fprintf(stderr, "epoch %d  rate %.2e  train %.6f  validation %.6f  (%.0f s)\n", epoch, rate,
                 sum / static_cast<double>(training.size() / batch_size * batch_size), validation_loss(network, validation, lambda), seconds);
    write_network(network, out_path);
  }

  torch::save(network, out_path + ".pt");
  check_quantization(network, validation, out_path);
  std::cerr << "network written to " << out_path << '\n';
  return 0;
}

} // namespace

int main(const int argc, char** argv) {
  if(argc < 3) {
    std::cerr << "usage: " << argv[0] << " DATA[,DATA...] OUT.nnue [epochs] [threads] [lambda] [seed] [hidden]\n";
    return 2;
  }
  const int epochs = argc > 3 ? std::stoi(argv[3]) : 20;
  const int threads = argc > 4 ? std::stoi(argv[4]) : 8;
  const float lambda = argc > 5 ? std::stof(argv[5]) : 0.75f;
  const unsigned seed = argc > 6 ? static_cast<unsigned>(std::stoul(argv[6])) : 1u;
  const int hidden = argc > 7 ? std::stoi(argv[7]) : 256;
  if(hidden <= 0 || hidden % 32 != 0 || hidden > NnueArch::max_hidden) {
    std::cerr << "hidden must be a multiple of 32 up to " << NnueArch::max_hidden << '\n';
    return 2;
  }
  return run(argv[1], argv[2], epochs, threads, lambda, seed, hidden);
}
