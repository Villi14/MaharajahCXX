#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace maharajah {

// Staged NNUE integration: weights can be loaded and reported, but there is no
// inference backend yet, so evaluation always falls back to the classic path.
class Nnue {
  public:
  [[nodiscard]] bool has_loaded_weights() const {
    return !weights_.empty();
  }

  [[nodiscard]] static bool backend_ready() {
    return false;
  }

  bool load_weights(const char* path);
  bool load_weights_from_bytes(const unsigned char* bytes, std::size_t size, const char* weights_version_name);
  void unload_weights();

  [[nodiscard]] const std::string& weights_version() const {
    return weights_version_;
  }

  [[nodiscard]] const std::string& loaded_path() const {
    return loaded_path_;
  }

  private:
  bool set_loaded_blob(const unsigned char* bytes, std::size_t size, const char* weights_version_name, const char* path);

  std::vector<unsigned char> weights_;
  std::string weights_version_;
  std::string loaded_path_;
};

} // namespace maharajah
