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
//     --voxel-representations=dense,block,svo,svdag   (также dense-brick — плотная сетка кирпичами 8^3)
//     --voxel-traversal=stack                спуск по SVO/SVDAG: stack, restart или оба через запятую
//     --voxel-views=3                        число фиксированных ракурсов
//     --voxel-warmup=30 --voxel-series=10 --voxel-frames=20
//     --voxel-cpu-stride=4                   шаг выборки пикселей для CPU-подсчёта шагов на луч
//     --voxel-report=voxel_experiment.csv
//     --voxel-view-only                      без замеров: сразу просмотр сцен
// Пути с пробелами заключаются в кавычки: --voxel-model="C:\models\my model.binvox".
namespace gpu_renderer::voxel {
struct ExperimentConfig {
  std::vector<SceneKind> scenes{SceneKind::kSphere, SceneKind::kTerrain};
  std::vector<std::uint32_t> resolutions{256u, 512u, 1024u};
  std::vector<std::filesystem::path> models{};
  std::vector<Representation> representations{Representation::kDense, Representation::kBlock,
                                               Representation::kSvo, Representation::kSvdag};
  std::vector<Traversal> traversals{Traversal::kStack};
  std::size_t views = 3u;
  std::uint32_t warmup_frames = 30u;
  std::uint32_t warmup_ms = 1000u;  // прогрев длится не меньше этого времени (--voxel-warmup-ms)
  std::uint32_t series = 10u;
  std::uint32_t frames_per_series = 20u;
  // CPU-обход выполняется для каждого cpu_stride-го пикселя по обеим осям:
  // среднее число шагов на луч от этого практически не меняется, а время подготовки
  // при 1920x1080 сокращается в cpu_stride^2 раз.
  std::uint32_t cpu_stride = 4u;
  std::filesystem::path report_path{"voxel_experiment.csv"};
  bool view_only = false;
};

// Сцены эксперимента в порядке прогона: процедурные сцены x разрешения, затем модели.
[[nodiscard]] std::vector<SceneCase> BuildSceneCases(ExperimentConfig const& config);

// Измеряемый вариант: представление и способ спуска. Способ спуска перебирается только
// для SVO и SVDAG; у остальных представлений один вариант (hierarchical = false).
struct MeasuredVariant {
  Representation representation;
  Traversal traversal;
  bool hierarchical;
};

// Варианты в порядке прогона: представления из config, для SVO и SVDAG — каждый способ спуска.
[[nodiscard]] std::vector<MeasuredVariant> BuildVariants(ExperimentConfig const& config);

[[nodiscard]] bool IsExperimentRequested(std::wstring_view command_line);

// Бросает std::invalid_argument при неверной опции.
[[nodiscard]] ExperimentConfig ParseExperimentConfig(std::wstring_view command_line);
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_EXPERIMENT_CONFIG_HPP
