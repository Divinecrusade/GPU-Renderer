#include "RayTraversal.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace gpu_renderer::voxel {
namespace {
constexpr std::uint32_t kMaxSteps = 16384u;  // совпадает с MAX_STEPS в шейдере

[[nodiscard]] Float3 operator+(Float3 a, Float3 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
[[nodiscard]] Float3 operator-(Float3 a, Float3 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
[[nodiscard]] Float3 operator*(Float3 a, float s) noexcept { return {a.x * s, a.y * s, a.z * s}; }
[[nodiscard]] Float3 Cross(Float3 a, Float3 b) noexcept {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
[[nodiscard]] Float3 Normalize(Float3 a) noexcept {
  float const length = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
  return a * (1.f / length);
}
[[nodiscard]] float Component(Float3 const& v, int axis) noexcept {
  return axis == 0 ? v.x : (axis == 1 ? v.y : v.z);
}

struct Int3 {
  int v[3];
};

[[nodiscard]] bool Outside(Int3 const& c, int n) noexcept {
  return c.v[0] < 0 || c.v[1] < 0 || c.v[2] < 0 || c.v[0] >= n || c.v[1] >= n || c.v[2] >= n;
}

[[nodiscard]] bool IntersectBox(Float3 o, Float3 inv, float n, float& t0, float& t1) noexcept {
  t0 = 0.f;
  t1 = 3.4e38f;
  for (int axis = 0; axis < 3; ++axis) {
    float const ta = (0.f - Component(o, axis)) * Component(inv, axis);
    float const tb = (n - Component(o, axis)) * Component(inv, axis);
    t0 = std::max(t0, std::min(ta, tb));
    t1 = std::min(t1, std::max(ta, tb));
  }
  return t0 <= t1;
}

[[nodiscard]] Int3 StartCell(Float3 o, Float3 d, float t, int n) noexcept {
  Int3 c{};
  for (int axis = 0; axis < 3; ++axis) {
    c.v[axis] = std::clamp(static_cast<int>(std::floor(Component(o, axis) + Component(d, axis) * t)), 0, n - 1);
  }
  return c;
}

[[nodiscard]] RayHit TraceDda(IVoxelGrid const& grid, Float3 o, Float3 d) {
  RayHit result;
  int const n = static_cast<int>(grid.Resolution());
  Float3 const inv{1.f / d.x, 1.f / d.y, 1.f / d.z};
  float t0 = 0.f;
  float t1 = 0.f;
  if (!IntersectBox(o, inv, static_cast<float>(n), t0, t1)) {
    return result;
  }
  Int3 c = StartCell(o, d, t0, n);
  int step[3];
  float t_max[3];
  float t_delta[3];
  for (int axis = 0; axis < 3; ++axis) {
    float const da = Component(d, axis);
    step[axis] = da > 0.f ? 1 : -1;
    t_max[axis] = (static_cast<float>(c.v[axis] + (da > 0.f ? 1 : 0)) - Component(o, axis)) * Component(inv, axis);
    t_delta[axis] = std::fabs(Component(inv, axis));
  }
  for (std::uint32_t i = 0u; i < kMaxSteps; ++i) {
    ++result.steps;
    if (grid.Get(static_cast<std::uint32_t>(c.v[0]), static_cast<std::uint32_t>(c.v[1]),
                 static_cast<std::uint32_t>(c.v[2]))) {
      result.hit = true;
      result.x = static_cast<std::uint32_t>(c.v[0]);
      result.y = static_cast<std::uint32_t>(c.v[1]);
      result.z = static_cast<std::uint32_t>(c.v[2]);
      return result;
    }
    int axis = 2;
    if (t_max[0] < t_max[1] && t_max[0] < t_max[2]) {
      axis = 0;
    } else if (t_max[1] < t_max[2]) {
      axis = 1;
    }
    c.v[axis] += step[axis];
    t_max[axis] += t_delta[axis];
    if (Outside(c, n)) {
      return result;
    }
  }
  return result;
}

// Выход из наибольшего пустого куба за один шаг; ячейка на оси выхода вычисляется целочисленно.
[[nodiscard]] RayHit TraceHierarchical(IVoxelGrid const& grid, Float3 o, Float3 d) {
  RayHit result;
  int const n = static_cast<int>(grid.Resolution());
  Float3 const inv{1.f / d.x, 1.f / d.y, 1.f / d.z};
  float t0 = 0.f;
  float t1 = 0.f;
  if (!IntersectBox(o, inv, static_cast<float>(n), t0, t1)) {
    return result;
  }
  float t = t0;
  Int3 c = StartCell(o, d, t, n);
  for (std::uint32_t i = 0u; i < kMaxSteps; ++i) {
    std::uint32_t const size = grid.EmptyCell(static_cast<std::uint32_t>(c.v[0]), static_cast<std::uint32_t>(c.v[1]),
                                              static_cast<std::uint32_t>(c.v[2]));
    ++result.steps;
    if (size == 0u) {
      result.hit = true;
      result.x = static_cast<std::uint32_t>(c.v[0]);
      result.y = static_cast<std::uint32_t>(c.v[1]);
      result.z = static_cast<std::uint32_t>(c.v[2]);
      return result;
    }
    int const s = static_cast<int>(size);
    int bound[3];
    float tb[3];
    for (int axis = 0; axis < 3; ++axis) {
      bound[axis] = (c.v[axis] / s) * s + (Component(d, axis) > 0.f ? s : 0);
      tb[axis] = (static_cast<float>(bound[axis]) - Component(o, axis)) * Component(inv, axis);
    }
    int const axis = (tb[0] < tb[1]) ? ((tb[0] < tb[2]) ? 0 : 2) : ((tb[1] < tb[2]) ? 1 : 2);
    t = std::max(t, tb[axis]);
    if (t >= t1) {
      return result;
    }
    // Луч движется монотонно, поэтому координата ячейки по каждой оси не может откатиться назад.
    // Без этого ограничения на ребре, где две границы пересекаются при одном t (с точностью float),
    // floor по второй оси возвращает луч в предыдущую ячейку, и обход зацикливается.
    for (int a = 0; a < 3; ++a) {
      int const next = static_cast<int>(std::floor(Component(o, a) + Component(d, a) * t));
      c.v[a] = Component(d, a) > 0.f ? std::max(next, c.v[a]) : std::min(next, c.v[a]);
    }
    c.v[axis] = Component(d, axis) > 0.f ? bound[axis] : bound[axis] - 1;
    if (Outside(c, n)) {
      return result;
    }
  }
  return result;
}
}  // namespace

CameraBasis MakeCameraBasis(Camera const& camera, float aspect) noexcept {
  Float3 const forward = Normalize(camera.target - camera.eye);
  Float3 const world_up{0.f, 1.f, 0.f};
  Float3 const right = Normalize(Cross(world_up, forward));
  Float3 const up = Cross(forward, right);
  float const tan_half = std::tan(camera.vertical_fov_rad * 0.5f);
  return {camera.eye, forward, right * (tan_half * aspect), up * tan_half};
}

Float3 PrimaryRayDirection(CameraBasis const& basis, std::uint32_t px, std::uint32_t py, std::uint32_t width,
                           std::uint32_t height) noexcept {
  float const ndc_x = (static_cast<float>(px) + 0.5f) / static_cast<float>(width) * 2.f - 1.f;
  float const ndc_y = 1.f - (static_cast<float>(py) + 0.5f) / static_cast<float>(height) * 2.f;
  Float3 dir = Normalize(basis.forward + basis.right * ndc_x + basis.up * ndc_y);
  // исключить деление на ноль в 1/d (как в шейдере)
  if (std::fabs(dir.x) < 1e-7f) dir.x += 1e-7f;
  if (std::fabs(dir.y) < 1e-7f) dir.y += 1e-7f;
  if (std::fabs(dir.z) < 1e-7f) dir.z += 1e-7f;
  return dir;
}

Camera MakeOrbitCamera(std::uint32_t resolution, std::size_t view_index, std::size_t view_count) noexcept {
  return MakeOrbitCameraAt(resolution, 0.6f + 2.f * std::numbers::pi_v<float> * static_cast<float>(view_index) /
                                                  static_cast<float>(std::max<std::size_t>(1u, view_count)));
}

Camera MakeOrbitCameraAt(std::uint32_t resolution, float azimuth) noexcept {
  float const n = static_cast<float>(resolution);
  float const elevation = 0.6f;  // ~34 градуса
  float const distance = 1.6f * n;
  Float3 const target{0.5f * n, 0.35f * n, 0.5f * n};
  Float3 const eye{target.x + distance * std::cos(elevation) * std::cos(azimuth),
                   target.y + distance * std::sin(elevation),
                   target.z + distance * std::cos(elevation) * std::sin(azimuth)};
  return {eye, target, 1.0f};  // ~57 градусов по вертикали
}

RayHit TraceRay(IVoxelGrid const& grid, Float3 origin, Float3 direction) {
  if (grid.GetRepresentation() == Representation::kDense) {
    return TraceDda(grid, origin, direction);
  }
  return TraceHierarchical(grid, origin, direction);
}

ImageTraceStats TraceImage(IVoxelGrid const& grid, CameraBasis const& basis, std::uint32_t width,
                           std::uint32_t height, std::uint32_t stride) {
  stride = std::max(1u, stride);
  double steps = 0.0;
  double hits = 0.0;
  std::size_t rays = 0u;
  std::uint32_t max_steps = 0u;
  for (std::uint32_t py = stride / 2u; py < height; py += stride) {
    for (std::uint32_t px = stride / 2u; px < width; px += stride) {
      RayHit const hit = TraceRay(grid, basis.position, PrimaryRayDirection(basis, px, py, width, height));
      steps += hit.steps;
      hits += hit.hit ? 1.0 : 0.0;
      max_steps = std::max(max_steps, hit.steps);
      ++rays;
    }
  }
  if (rays == 0u) {
    return {};
  }
  return {steps / static_cast<double>(rays), hits / static_cast<double>(rays), rays, max_steps};
}
}  // namespace gpu_renderer::voxel
