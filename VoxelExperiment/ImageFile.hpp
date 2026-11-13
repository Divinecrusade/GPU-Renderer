#ifndef VOXEL_IMAGE_FILE_HPP
#define VOXEL_IMAGE_FILE_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

namespace gpu_renderer::voxel {
// Порядок каналов в пикселе исходного изображения (4 байта на пиксель).
enum class PixelOrder { kRgba, kBgra };

// Записывает изображение в несжатый 24-битный BMP (альфа-канал отбрасывается).
// pixels — строки сверху вниз, row_pitch — шаг строки в байтах (>= 4 * width).
// Без зависимостей от Windows: формат читается Pillow, Word и любым редактором.
void WriteBmp24(std::filesystem::path const& path, std::uint32_t width, std::uint32_t height,
                std::span<std::uint8_t const> pixels, std::size_t row_pitch, PixelOrder order);
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_IMAGE_FILE_HPP
