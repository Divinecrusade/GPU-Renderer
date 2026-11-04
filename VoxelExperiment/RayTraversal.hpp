#ifndef VOXEL_RAY_TRAVERSAL_HPP
#define VOXEL_RAY_TRAVERSAL_HPP

#include <cstddef>
#include <cstdint>

#include "VoxelGrid.hpp"

// CPU-копия обхода лучей из VoxelTracePS.hlsl (те же формулы, тот же float).
// Нужна для аппаратно-независимой метрики «шагов на луч» и для проверки шейдера.
namespace gpu_renderer::voxel {
struct Float3 {
  float x;
  float y;
  float z;
};

// Камера в координатах сетки: сцена занимает куб [0, N]^3, ось y направлена вверх.
struct Camera {
  Float3 eye;
  Float3 target;
  float vertical_fov_rad;
};

// Базис для генерации лучей: dir = normalize(forward + ndc.x * right + ndc.y * up),
// где right и up уже умножены на tan(fov / 2) (и right — на соотношение сторон).
struct CameraBasis {
  Float3 position;
  Float3 forward;
  Float3 right;
  Float3 up;
};

[[nodiscard]] CameraBasis MakeCameraBasis(Camera const& camera, float aspect) noexcept;

// Направление первичного луча через центр пикселя (px + 0.5, py + 0.5), как SV_Position.
[[nodiscard]] Float3 PrimaryRayDirection(CameraBasis const& basis, std::uint32_t px, std::uint32_t py,
                                         std::uint32_t width, std::uint32_t height) noexcept;

// Камера на орбите вокруг сцены под азимутом azimuth_rad (высота ~34 градуса, расстояние 1.6N).
[[nodiscard]] Camera MakeOrbitCameraAt(std::uint32_t resolution, float azimuth_rad) noexcept;

// Фиксированный ракурс эксперимента: view_index из view_count, равномерно по азимуту.
[[nodiscard]] Camera MakeOrbitCamera(std::uint32_t resolution, std::size_t view_index,
                                     std::size_t view_count) noexcept;

struct RayHit {
  bool hit = false;
  std::uint32_t steps = 0u;
  std::uint32_t x = 0u;
  std::uint32_t y = 0u;
  std::uint32_t z = 0u;
};

// 3D-DDA для плотной сетки, иерархический обход (EmptyCell) — для остальных.
[[nodiscard]] RayHit TraceRay(IVoxelGrid const& grid, Float3 origin, Float3 direction);

struct ImageTraceStats {
  double mean_steps = 0.0;
  double hit_rate = 0.0;
};

// Обход всех пикселей кадра width x height; результат — среднее число шагов и доля попаданий.
[[nodiscard]] ImageTraceStats TraceImage(IVoxelGrid const& grid, CameraBasis const& basis,
                                         std::uint32_t width, std::uint32_t height);
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_RAY_TRAVERSAL_HPP
