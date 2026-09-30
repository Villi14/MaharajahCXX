#pragma once

#include <chrono>

namespace maharajah {

// Milliseconds since the first call.
[[nodiscard]] inline int now_ms() {
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  return static_cast<int>(duration_cast<milliseconds>(steady_clock::now() - start).count());
}

} // namespace maharajah
