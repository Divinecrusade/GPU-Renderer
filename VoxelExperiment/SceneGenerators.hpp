#ifndef VOXEL_SCENE_GENERATORS_HPP
#define VOXEL_SCENE_GENERATORS_HPP

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Тестовые сцены. Результат каждого генератора — отсортированные Мортон-коды
// граничных вокселей (занятых вокселей, у которых есть пустой сосед по грани).
namespace gpu_renderer::voxel {
enum class SceneKind { kSphere, kTerrain, kModel };

[[nodiscard]] std::string_view ToString(SceneKind scene) noexcept;

// Сфера радиуса radius (в вокселях) с центром N/2; O(N^2 + n) по столбцам.
[[nodiscard]] std::vector<std::uint64_t> GenerateSphereShell(std::uint32_t resolution, double radius);

// Ландшафт: chi(x,y,z) = [y < H(x,z)] && !cave(x,y,z).
// H — двумерный фрактальный шум; тоннели — пересечение тонких нулевых слоёв двух
// трёхмерных шумов (|n1| < tau && |n2| < tau) в полосе depth под поверхностью.
struct TerrainParams {
  double base_height = 0.35;   // доля N
  double amplitude = 0.5;      // доля N
  double frequency = 3.0;      // периодов рельефа на сторону сетки
  double cave_depth = 0.15;    // доля N
  double cave_frequency = 5.0;
  double cave_width = 0.08;    // tau
  std::uint32_t seed = 1u;
};

// threads = 0 — по числу аппаратных потоков.
[[nodiscard]] std::vector<std::uint64_t> GenerateTerrainShell(std::uint32_t resolution,
                                                              TerrainParams const& params = {},
                                                              unsigned threads = 0u);

// Модель в формате binvox (например, выход cuda_voxelizer с флагом -o binvox).
// Размер дополняется до степени двойки; извлекается оболочка по 6-соседству,
// поэтому годится и поверхностная, и сплошная (-solid) вокселизация.
struct VoxelModel {
  std::uint32_t resolution = 0u;
  std::vector<std::uint64_t> shell_codes;
};

[[nodiscard]] VoxelModel LoadBinvox(std::filesystem::path const& path);

// Запись занятых вокселей в binvox (для тестов и обмена с внешними инструментами).
void SaveBinvox(std::filesystem::path const& path, std::uint32_t resolution,
                std::span<std::uint64_t const> sorted_codes);

// Одна сцена эксперимента: процедурная (kind + resolution) или модель (kind = kModel + путь).
struct SceneCase {
  SceneKind kind = SceneKind::kSphere;
  std::uint32_t resolution = 0u;  // 0 для модели — берётся из файла
  std::filesystem::path model_path{};
};

struct LoadedScene {
  std::uint32_t resolution = 0u;
  std::vector<std::uint64_t> shell_codes;
  double generation_ms = 0.0;
};

// Генерация или загрузка сцены; бросает исключение, если модель не читается.
// Сфера: радиус N/2 - 1; ландшафт: параметры TerrainParams по умолчанию.
[[nodiscard]] LoadedScene LoadSceneCase(SceneCase const& scene);

// Имя сцены для отчёта: "sphere", "terrain", "model:<имя файла>".
[[nodiscard]] std::string SceneCaseName(SceneCase const& scene);
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_SCENE_GENERATORS_HPP
