#ifndef VOXEL_EXPERIMENT_CONFIG_HPP
#define VOXEL_EXPERIMENT_CONFIG_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

#include "SceneGenerators.hpp"
#include "VoxelGrid.hpp"

// Параметры вычислительного эксперимента и их разбор из командной строки:
//   GPU-Renderer.exe --voxel-experiment [опции]
//     --voxel-scenes=sphere,terrain          процедурные сцены (none — только модели)
//     --voxel-resolutions=256,512,1024       разрешения N для процедурных сцен
//     --voxel-model=path.binvox              модель (можно повторять; N берётся из файла)
//     --voxel-representations=dense,block,svo,svdag
//     --voxel-views=3                        число фиксированных ракурсов
//     --voxel-warmup=30 --voxel-series=10 --voxel-frames=20
//     --voxel-report=voxel_experiment.csv
// Пути с пробелами заключаются в кавычки: --voxel-model="C:\models\my model.binvox".
namespace gpu_renderer::voxel {
struct ExperimentConfig {
  std::vector<SceneKind> scenes{SceneKind::kSphere, SceneKind::kTerrain};
  std::vector<std::uint32_t> resolutions{256u, 512u, 1024u};
  std::vector<std::filesystem::path> models{};
  std::vector<Representation> representations{Representation::kDense, Representation::kBlock,
                                               Representation::kSvo, Representation::kSvdag};
  std::size_t views = 3u;
  std::uint32_t warmup_frames = 30u;
  std::uint32_t series = 10u;
  std::uint32_t frames_per_series = 20u;
  std::filesystem::path report_path{"voxel_experiment.csv"};
};

[[nodiscard]] bool IsExperimentRequested(std::wstring_view command_line);

// Бросает std::invalid_argument при неверной опции.
[[nodiscard]] ExperimentConfig ParseExperimentConfig(std::wstring_view command_line);
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_EXPERIMENT_CONFIG_HPP
