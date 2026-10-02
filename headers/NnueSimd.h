#pragma once

// Vector helpers of the NNUE evaluation. Row add/sub/copy adapted from simd.h of
// https://github.com/jdart1/nnue (d8c1d3c):
//
//   Copyright (c) 2021, 2022 Jon Dart. MIT License: permission is hereby granted, free
//   of charge, to any person obtaining a copy of this software and associated
//   documentation files, to deal in the Software without restriction, including the
//   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//   copies, subject to the condition that the above copyright notice and this
//   permission notice are included in all copies or substantial portions of the
//   Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
//
// Changes: the instruction set follows the compiler's own macros (__AVX512BW__,
// __AVX2__, __SSE2__, __ARM_NEON) instead of build flags, a scalar fallback (wasm,
// other targets) replaces the #error, and screlu_dot (the output layer) is new.
// Every array is `size` int16 values, 64-byte aligned, size a multiple of 32.

#include <cstddef>
#include <cstdint>

#if defined(__AVX512BW__)
#define MAHARAJAH_NNUE_AVX512
#include <immintrin.h>
#elif defined(__AVX2__)
#define MAHARAJAH_NNUE_AVX2
#include <immintrin.h>
#elif defined(__SSE2__) || defined(_M_X64)
#define MAHARAJAH_NNUE_SSE2
#include <emmintrin.h>
#elif defined(__ARM_NEON)
#define MAHARAJAH_NNUE_NEON
#include <arm_neon.h>
#endif

namespace maharajah::nnue_simd {

// out += in
inline void vec_add(const int16_t* in, int16_t* out, const std::size_t size) {
#if defined(MAHARAJAH_NNUE_AVX512)
  for(std::size_t i{ }; i < size; i += 32)
    _mm512_store_si512(out + i, _mm512_add_epi16(_mm512_load_si512(out + i), _mm512_load_si512(in + i)));
#elif defined(MAHARAJAH_NNUE_AVX2)
  auto* outp = reinterpret_cast<__m256i*>(out);
  const auto* inp = reinterpret_cast<const __m256i*>(in);
  for(std::size_t i{ }; i < size / 16; ++i)
    outp[i] = _mm256_add_epi16(outp[i], inp[i]);
#elif defined(MAHARAJAH_NNUE_SSE2)
  auto* outp = reinterpret_cast<__m128i*>(out);
  const auto* inp = reinterpret_cast<const __m128i*>(in);
  for(std::size_t i{ }; i < size / 8; ++i)
    outp[i] = _mm_add_epi16(outp[i], inp[i]);
#elif defined(MAHARAJAH_NNUE_NEON)
  for(std::size_t i{ }; i < size; i += 8)
    vst1q_s16(out + i, vaddq_s16(vld1q_s16(out + i), vld1q_s16(in + i)));
#else
  for(std::size_t i{ }; i < size; ++i)
    out[i] = static_cast<int16_t>(out[i] + in[i]);
#endif
}

// out -= in
inline void vec_sub(const int16_t* in, int16_t* out, const std::size_t size) {
#if defined(MAHARAJAH_NNUE_AVX512)
  for(std::size_t i{ }; i < size; i += 32)
    _mm512_store_si512(out + i, _mm512_sub_epi16(_mm512_load_si512(out + i), _mm512_load_si512(in + i)));
#elif defined(MAHARAJAH_NNUE_AVX2)
  auto* outp = reinterpret_cast<__m256i*>(out);
  const auto* inp = reinterpret_cast<const __m256i*>(in);
  for(std::size_t i{ }; i < size / 16; ++i)
    outp[i] = _mm256_sub_epi16(outp[i], inp[i]);
#elif defined(MAHARAJAH_NNUE_SSE2)
  auto* outp = reinterpret_cast<__m128i*>(out);
  const auto* inp = reinterpret_cast<const __m128i*>(in);
  for(std::size_t i{ }; i < size / 8; ++i)
    outp[i] = _mm_sub_epi16(outp[i], inp[i]);
#elif defined(MAHARAJAH_NNUE_NEON)
  for(std::size_t i{ }; i < size; i += 8)
    vst1q_s16(out + i, vsubq_s16(vld1q_s16(out + i), vld1q_s16(in + i)));
#else
  for(std::size_t i{ }; i < size; ++i)
    out[i] = static_cast<int16_t>(out[i] - in[i]);
#endif
}

// out = in
inline void vec_copy(const int16_t* in, int16_t* out, const std::size_t size) {
  for(std::size_t i{ }; i < size; ++i)
    out[i] = in[i];
}

// Output layer with SCReLU: sum of clamp(acc, 0, qa)^2 * weight. clamp * weight fits
// int16 (qa 255, |weight| <= 127); the products are summed in int32.
inline int32_t screlu_dot(const int16_t* acc, const int16_t* weights, const std::size_t size, const int16_t qa) {
#if defined(MAHARAJAH_NNUE_AVX512)
  const __m512i zero = _mm512_setzero_si512(), max = _mm512_set1_epi16(qa);
  __m512i sum = _mm512_setzero_si512();
  for(std::size_t i{ }; i < size; i += 32) {
    const __m512i v = _mm512_min_epi16(_mm512_max_epi16(_mm512_load_si512(acc + i), zero), max);
    sum = _mm512_add_epi32(sum, _mm512_madd_epi16(_mm512_mullo_epi16(v, _mm512_load_si512(weights + i)), v));
  }
  return _mm512_reduce_add_epi32(sum);
#elif defined(MAHARAJAH_NNUE_AVX2)
  const __m256i zero = _mm256_setzero_si256(), max = _mm256_set1_epi16(qa);
  __m256i sum = _mm256_setzero_si256();
  for(std::size_t i{ }; i < size; i += 16) {
    const __m256i v = _mm256_min_epi16(_mm256_max_epi16(_mm256_load_si256(reinterpret_cast<const __m256i*>(acc + i)), zero), max);
    const __m256i w = _mm256_load_si256(reinterpret_cast<const __m256i*>(weights + i));
    sum = _mm256_add_epi32(sum, _mm256_madd_epi16(_mm256_mullo_epi16(v, w), v));
  }
  __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
  sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, 0x4E));
  sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, 0xB1));
  return _mm_cvtsi128_si32(sum128);
#elif defined(MAHARAJAH_NNUE_SSE2)
  const __m128i zero = _mm_setzero_si128(), max = _mm_set1_epi16(qa);
  __m128i sum = _mm_setzero_si128();
  for(std::size_t i{ }; i < size; i += 8) {
    const __m128i v = _mm_min_epi16(_mm_max_epi16(_mm_load_si128(reinterpret_cast<const __m128i*>(acc + i)), zero), max);
    const __m128i w = _mm_load_si128(reinterpret_cast<const __m128i*>(weights + i));
    sum = _mm_add_epi32(sum, _mm_madd_epi16(_mm_mullo_epi16(v, w), v));
  }
  sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4E));
  sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0xB1));
  return _mm_cvtsi128_si32(sum);
#elif defined(MAHARAJAH_NNUE_NEON)
  const int16x8_t zero = vdupq_n_s16(0), max = vdupq_n_s16(qa);
  int32x4_t sum = vdupq_n_s32(0);
  for(std::size_t i{ }; i < size; i += 8) {
    const int16x8_t v = vminq_s16(vmaxq_s16(vld1q_s16(acc + i), zero), max);
    const int16x8_t product = vmulq_s16(v, vld1q_s16(weights + i));
    sum = vmlal_s16(sum, vget_low_s16(product), vget_low_s16(v));
    sum = vmlal_s16(sum, vget_high_s16(product), vget_high_s16(v));
  }
#if defined(__aarch64__)
  return vaddvq_s32(sum);
#else
  return vgetq_lane_s32(sum, 0) + vgetq_lane_s32(sum, 1) + vgetq_lane_s32(sum, 2) + vgetq_lane_s32(sum, 3);
#endif
#else
  int32_t sum{ };
  for(std::size_t i{ }; i < size; ++i) {
    const int32_t v = acc[i] < 0 ? 0 : (acc[i] > qa ? qa : acc[i]);
    sum += static_cast<int16_t>(v * weights[i]) * v;
  }
  return sum;
#endif
}

} // namespace maharajah::nnue_simd
