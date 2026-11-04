#ifndef VOXEL_VOLUME_HPP
#define VOXEL_VOLUME_HPP

#include <cstdint>

#include "../ConstBuffer.hpp"
#include "../DrawableBase.hpp"
#include "RayTraversal.hpp"
#include "VoxelGrid.hpp"

namespace gpu_renderer::drawable {
// Зеркало cbuffer TraceParams из VoxelTracePS.hlsl (5 регистров по 16 байт).
struct TraceParams {
  voxel::Float3 camera_position;
  std::uint32_t resolution;
  voxel::Float3 forward;
  std::uint32_t depth;
  voxel::Float3 right;
  std::uint32_t mode;
  voxel::Float3 up;
  std::uint32_t show_steps;
  std::uint32_t width;
  std::uint32_t height;
  std::uint32_t blocks_per_side;
  std::uint32_t directory_size;
};
static_assert(sizeof(TraceParams) == 80u, "TraceParams must match the HLSL constant buffer layout");

// Воксельная сцена, визуализируемая трассировкой лучей в пиксельном шейдере
// поверх полноэкранного прямоугольника. Общие Bindable: геометрия прямоугольника и шейдеры;
// собственные: StructuredBuffer с данными представления (t0) и константы трассировки (b0).
class VoxelVolume : public DrawableBase<VoxelVolume> {
 public:
  // Плотная сетка в шейдере индексируется 32-битным x + N(y + Nz).
  static constexpr std::uint32_t kMaxDenseResolution = 1024u;

  VoxelVolume(Graphics& gfx, voxel::IVoxelGrid const& grid, std::uint32_t viewport_width,
              std::uint32_t viewport_height);

  // Камера в координатах сетки; show_steps — тепловая карта числа шагов обхода вместо затенения.
  void SetView(Graphics& gfx, voxel::CameraBasis const& basis, bool show_steps);

  void Update(FrameTimer::DeltaTime dt) override;
  DirectX::XMMATRIX GetTransformation() const override;

 private:
  TraceParams params_;
  std::shared_ptr<bindable::PixelConstBuffer<TraceParams>> params_buffer_;
};
}  // namespace gpu_renderer::drawable

#endif  // !VOXEL_VOLUME_HPP
