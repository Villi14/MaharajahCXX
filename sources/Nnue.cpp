#include "../headers/Nnue.h"

#include "../headers/Bitboard.h"
#include "../headers/NnueSimd.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string_view>

namespace maharajah {

namespace {

constexpr int hidden{ NnueArch::hidden };

std::uint32_t read_u32(const unsigned char* bytes) {
  return static_cast<std::uint32_t>(bytes[0]) | static_cast<std::uint32_t>(bytes[1]) << 8 | static_cast<std::uint32_t>(bytes[2]) << 16 |
         static_cast<std::uint32_t>(bytes[3]) << 24;
}

// reads `count` little-endian int16 values
void read_i16(const unsigned char*& bytes, int16_t* out, const std::size_t count) {
  for(std::size_t i{ }; i < count; ++i, bytes += 2)
    out[i] = static_cast<int16_t>(static_cast<std::uint16_t>(bytes[0] | bytes[1] << 8));
}

} // namespace

struct alignas(64) Nnue::Params {
  std::array<std::array<int16_t, hidden>, NnueArch::inputs> feature_weights;
  std::array<int16_t, hidden> feature_bias;
  std::array<int16_t, 2 * hidden> output_weights;
  int32_t output_bias;
};

Nnue::Nnue() = default;
Nnue::~Nnue() = default;

bool Nnue::set_loaded_blob(const unsigned char* bytes, const std::size_t size, const char* weights_version_name, const char* path) {
  unload_weights();
  if(bytes == nullptr || size != NnueArch::file_bytes || std::memcmp(bytes, "MHNN", 4) != 0 || read_u32(bytes + 4) != NnueArch::version ||
     read_u32(bytes + 8) != NnueArch::inputs || read_u32(bytes + 12) != hidden)
    return false;

  auto params = std::make_unique<Params>();
  const unsigned char* cursor = bytes + NnueArch::header_bytes;
  for(auto& row : params->feature_weights)
    read_i16(cursor, row.data(), row.size());
  read_i16(cursor, params->feature_bias.data(), params->feature_bias.size());
  read_i16(cursor, params->output_weights.data(), params->output_weights.size());
  params->output_bias = static_cast<int32_t>(read_u32(cursor));

  params_ = std::move(params);
  loaded_path_ = path ? path : "";
  weights_version_ = (weights_version_name && *weights_version_name) ? weights_version_name : "memory";
  return true;
}

bool Nnue::load_weights(const char* path) {
  if(path == nullptr || *path == '\0')
    return false;

  std::ifstream file(path, std::ios::binary);
  const std::vector<unsigned char> bytes{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
  if(!file.good() && !file.eof()) {
    unload_weights();
    return false;
  }

  const std::string_view full_path{ path };
  const auto slash = full_path.find_last_of("/\\");
  const std::string file_name{ slash == std::string_view::npos ? full_path : full_path.substr(slash + 1) };
  return set_loaded_blob(bytes.data(), bytes.size(), file_name.c_str(), path);
}

bool Nnue::load_weights_from_bytes(const unsigned char* bytes, const std::size_t size, const char* weights_version_name) {
  return set_loaded_blob(bytes, size, weights_version_name, "<memory>");
}

void Nnue::unload_weights() {
  params_.reset();
  weights_version_.clear();
  loaded_path_.clear();
}

void Nnue::refresh(const BoardState& state, NnueAccumulator& accumulator) const {
  for(const Colors perspective : { white, black }) {
    int16_t* values = accumulator.values[perspective].data();
    nnue_simd::vec_copy<hidden>(params_->feature_bias.data(), values);
    for(int piece{ }; piece < PieceCount::all; ++piece) {
      for(u64 bitboard = state.bitboards[piece]; bitboard; pop_bit(bitboard, get_ls1b_index(bitboard)))
        nnue_simd::vec_add<hidden>(params_->feature_weights[NnueArch::feature(perspective, piece, get_ls1b_index(bitboard))].data(), values);
    }
  }
  accumulator.key = state.hash_key;
  accumulator.computed = true;
}

void Nnue::update(const BoardState& parent, const BoardState& state, const NnueAccumulator& from, NnueAccumulator& to) const {
  for(const Colors perspective : { white, black }) {
    int16_t* values = to.values[perspective].data();
    nnue_simd::vec_copy<hidden>(from.values[perspective].data(), values);
    for(int piece{ }; piece < PieceCount::all; ++piece) {
      const u64 before = parent.bitboards[piece], after = state.bitboards[piece];
      if(before == after)
        continue;
      for(u64 removed = before & ~after; removed; pop_bit(removed, get_ls1b_index(removed)))
        nnue_simd::vec_sub<hidden>(params_->feature_weights[NnueArch::feature(perspective, piece, get_ls1b_index(removed))].data(), values);
      for(u64 added = after & ~before; added; pop_bit(added, get_ls1b_index(added)))
        nnue_simd::vec_add<hidden>(params_->feature_weights[NnueArch::feature(perspective, piece, get_ls1b_index(added))].data(), values);
    }
  }
  to.key = state.hash_key;
  to.computed = true;
}

int Nnue::evaluate(const NnueAccumulator& accumulator, const Colors side) const {
  const int32_t sum = nnue_simd::screlu_dot<hidden>(accumulator.values[side].data(), params_->output_weights.data(), NnueArch::qa) +
                      nnue_simd::screlu_dot<hidden>(accumulator.values[opponent(side)].data(), params_->output_weights.data() + hidden, NnueArch::qa);
  const int64_t output = static_cast<int64_t>(sum / NnueArch::qa + params_->output_bias) * NnueArch::scale / (NnueArch::qa * NnueArch::qb);
  // well inside the mate scores
  return static_cast<int>(std::clamp<int64_t>(output, -30000, 30000));
}

int Nnue::evaluate(const BoardState& state) const {
  NnueAccumulator accumulator;
  refresh(state, accumulator);
  return evaluate(accumulator, state.side);
}

} // namespace maharajah
