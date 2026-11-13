#ifndef VOXEL_FRAME_CAPTURE_HPP
#define VOXEL_FRAME_CAPTURE_HPP

#include <filesystem>

#include "../Graphics.hpp"

namespace gpu_renderer::voxel {
// Сохранение заднего буфера (уже отрисованного кадра) в файл для рисунков статьи.
// Вызывать после отрисовки и до Graphics::EndFrame: при DXGI_SWAP_EFFECT_DISCARD
// содержимое буфера после Present не определено. Рамка и заголовок окна в кадр не попадают.
class FrameCapture final {
 public:
  FrameCapture() = delete;

  // Записывает кадр в 24-битный BMP. Блокирует CPU до завершения отрисовки (копия через
  // staging-текстуру), поэтому только для снимков, не для замеров времени.
  static void SaveBackBuffer(Graphics& gfx, std::filesystem::path const& path);
};
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_FRAME_CAPTURE_HPP
