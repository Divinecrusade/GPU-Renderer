#include "ImageFile.hpp"

#include <array>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace gpu_renderer::voxel {
namespace {
void PutLe(std::vector<std::uint8_t>& out, std::uint32_t value, std::size_t bytes) {
  for (std::size_t i = 0u; i < bytes; ++i) {
    out.push_back(static_cast<std::uint8_t>(value >> (8u * i)));
  }
}
}  // namespace

void WriteBmp24(std::filesystem::path const& path, std::uint32_t width, std::uint32_t height,
                std::span<std::uint8_t const> pixels, std::size_t row_pitch, PixelOrder order) {
  if (width == 0u || height == 0u || row_pitch < 4u * static_cast<std::size_t>(width) ||
      pixels.size() < row_pitch * (height - 1u) + 4u * static_cast<std::size_t>(width)) {
    throw std::invalid_argument{"WriteBmp24: image size does not match the pixel buffer"};
  }
  std::size_t const row_bytes = (3u * static_cast<std::size_t>(width) + 3u) / 4u * 4u;  // выравнивание до 4 байт
  std::size_t const data_bytes = row_bytes * height;
  constexpr std::uint32_t kHeaderBytes = 14u + 40u;

  std::vector<std::uint8_t> file;
  file.reserve(kHeaderBytes + data_bytes);
  file.push_back('B');
  file.push_back('M');
  PutLe(file, static_cast<std::uint32_t>(kHeaderBytes + data_bytes), 4u);
  PutLe(file, 0u, 4u);             // зарезервировано
  PutLe(file, kHeaderBytes, 4u);   // смещение данных
  PutLe(file, 40u, 4u);            // BITMAPINFOHEADER
  PutLe(file, width, 4u);
  PutLe(file, height, 4u);         // > 0: строки снизу вверх
  PutLe(file, 1u, 2u);             // плоскостей
  PutLe(file, 24u, 2u);            // бит на пиксель
  PutLe(file, 0u, 4u);             // BI_RGB, без сжатия
  PutLe(file, static_cast<std::uint32_t>(data_bytes), 4u);
  PutLe(file, 11811u, 4u);         // 300 dpi в пикселях на метр (по горизонтали)
  PutLe(file, 11811u, 4u);         // и по вертикали
  PutLe(file, 0u, 4u);
  PutLe(file, 0u, 4u);

  std::size_t const r = order == PixelOrder::kRgba ? 0u : 2u;
  std::size_t const b = order == PixelOrder::kRgba ? 2u : 0u;
  for (std::uint32_t y = height; y-- > 0u;) {
    std::uint8_t const* row = pixels.data() + row_pitch * y;
    for (std::uint32_t x = 0u; x < width; ++x) {
      std::uint8_t const* p = row + 4u * static_cast<std::size_t>(x);
      file.push_back(p[b]);  // BMP хранит B, G, R
      file.push_back(p[1]);
      file.push_back(p[r]);
    }
    file.resize(file.size() + (row_bytes - 3u * static_cast<std::size_t>(width)), 0u);
  }

  std::ofstream out{path, std::ios::binary | std::ios::trunc};
  out.write(reinterpret_cast<char const*>(file.data()), static_cast<std::streamsize>(file.size()));
  if (!out) {
    throw std::runtime_error{"WriteBmp24: cannot write " + path.string()};
  }
}
}  // namespace gpu_renderer::voxel
