#pragma once

#include "BoardState.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace maharajah {

// NNUE evaluation: (18 pieces x 64 squares) -> 256 per perspective -> 1, SCReLU.
// Every piece kind, compound pieces and kings included, is an input of its own. The
// side to move's accumulator comes first in the output layer; a black perspective
// sees the board flipped vertically and its own pieces as "own".
struct NnueArch {
  static constexpr int inputs{ PieceCount::all * 64 };
  static constexpr int hidden{ 256 };
  // quantization: feature weights and biases x qa, output weights x qb
  static constexpr int qa{ 255 };
  static constexpr int qb{ 64 };
  // network output x scale = centipawns
  static constexpr int scale{ 400 };
  static constexpr std::uint32_t version{ 1 };
  // "MHNN", version, inputs, hidden (u32 each), feature weights (inputs x hidden,
  // feature by feature), feature biases, output weights (2 x hidden: side to move,
  // then the other side), all int16, and the int32 output bias; little-endian
  static constexpr std::size_t header_bytes{ 16 };
  static constexpr std::size_t file_bytes{ header_bytes + 2 * (std::size_t{ inputs } * hidden + hidden + 2 * hidden) + 4 };

  // input index of `piece` on `square` seen from `perspective`
  [[nodiscard]] static constexpr int feature(const Colors perspective, const int piece, const int square) {
    const int kind = piece % PieceCount::per_side;
    const bool own = (piece < PieceCount::per_side) == (perspective == white);
    return ((own ? 0 : PieceCount::per_side) + kind) * 64 + (perspective == white ? square : square ^ 56);
  }
};

struct alignas(64) NnueAccumulator {
  std::array<std::array<int16_t, NnueArch::hidden>, 2> values;
  // hash key of the position these values belong to, valid when `computed`
  u64 key{ };
  bool computed{ };
};

class Nnue {
  public:
  Nnue();
  ~Nnue();
  Nnue(const Nnue&) = delete;
  Nnue& operator=(const Nnue&) = delete;

  [[nodiscard]] bool has_loaded_weights() const {
    return params_ != nullptr;
  }

  // compiled in on every target (with a scalar fallback)
  [[nodiscard]] static bool backend_ready() {
    return true;
  }

  // Loads a network file (format in NnueArch). A file of another size, magic or
  // shape is rejected and leaves the network unloaded.
  bool load_weights(const char* path);
  bool load_weights_from_bytes(const unsigned char* bytes, std::size_t size, const char* weights_version_name);
  void unload_weights();

  [[nodiscard]] const std::string& weights_version() const {
    return weights_version_;
  }

  [[nodiscard]] const std::string& loaded_path() const {
    return loaded_path_;
  }

  // Requires loaded weights.
  void refresh(const BoardState& state, NnueAccumulator& accumulator) const;
  // `to` = `from` (the accumulator of `parent`) updated for the pieces that differ
  // between `parent` and `state`
  void update(const BoardState& parent, const BoardState& state, const NnueAccumulator& from, NnueAccumulator& to) const;
  // centipawns from the side to move's point of view
  [[nodiscard]] int evaluate(const NnueAccumulator& accumulator, Colors side) const;
  [[nodiscard]] int evaluate(const BoardState& state) const;

  private:
  struct Params;

  bool set_loaded_blob(const unsigned char* bytes, std::size_t size, const char* weights_version_name, const char* path);

  std::unique_ptr<Params> params_;
  std::string weights_version_;
  std::string loaded_path_;
};

} // namespace maharajah
