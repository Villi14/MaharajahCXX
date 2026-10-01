#include "../headers/Nnue.h"

#include <fstream>
#include <iterator>
#include <string_view>

namespace maharajah {

bool Nnue::set_loaded_blob(const unsigned char* bytes, const std::size_t size, const char* weights_version_name, const char* path) {
  if(bytes == nullptr || size == 0)
    return false;

  weights_.assign(bytes, bytes + size);
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
  if(bytes.empty()) {
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
  weights_.clear();
  weights_version_.clear();
  loaded_path_.clear();
}

} // namespace maharajah
