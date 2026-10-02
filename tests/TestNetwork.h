#pragma once

#include "../headers/Nnue.h"

#include <cstdint>
#include <random>
#include <vector>

namespace maharajah {

// A network file of the engine's shape with random weights in the ranges a trained
// network keeps (feature weights and biases within +-qa, output weights within +-127).
inline std::vector<unsigned char> random_network_bytes(const unsigned seed, const int hidden = 256) {
  std::vector<unsigned char> bytes;
  bytes.reserve(NnueArch::file_bytes(hidden));
  const auto put_u32 = [&](const std::uint32_t value) {
    for(int shift{ }; shift < 32; shift += 8)
      bytes.push_back(static_cast<unsigned char>(value >> shift));
  };
  const auto put_i16 = [&](const int value) {
    const auto bits = static_cast<std::uint16_t>(static_cast<int16_t>(value));
    bytes.push_back(static_cast<unsigned char>(bits));
    bytes.push_back(static_cast<unsigned char>(bits >> 8));
  };

  std::mt19937 rng(seed);
  std::uniform_int_distribution feature(-NnueArch::qa / 4, NnueArch::qa / 4);
  std::uniform_int_distribution output(-127, 127);

  bytes.insert(bytes.end(), { 'M', 'H', 'N', 'N' });
  put_u32(NnueArch::version);
  put_u32(NnueArch::inputs);
  put_u32(hidden);
  for(int i{ }; i < NnueArch::inputs * hidden; ++i)
    put_i16(feature(rng));
  for(int i{ }; i < hidden; ++i)
    put_i16(feature(rng) + NnueArch::qa / 2);
  for(int i{ }; i < 2 * hidden; ++i)
    put_i16(output(rng));
  put_u32(static_cast<std::uint32_t>(std::uniform_int_distribution(-1000, 1000)(rng)));
  return bytes;
}

} // namespace maharajah
