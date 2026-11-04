#ifndef VOXEL_EXPERIMENT_RUNNER_HPP
#define VOXEL_EXPERIMENT_RUNNER_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include "../FrameTimer.hpp"
#include "../Graphics.hpp"
#include "ExperimentConfig.hpp"
#include "ExperimentReport.hpp"
#include "GpuTimer.hpp"
#include "RayTraversal.hpp"
#include "VoxelGrid.hpp"
#include "VoxelVolume.hpp"

namespace gpu_renderer::voxel {
// Автомат эксперимента, выполняемый по одному кадру за вызов RenderFrame.
// Для каждой пары (сцена, N) и каждого представления:
//   1) построение структуры на CPU (время, объём памяти);
//   2) для каждого ракурса — CPU-обход того же кадра (среднее число шагов на луч, доля попаданий),
//      затем на GPU warmup кадров и series x frames кадров с замером времени по timestamp-запросам;
//   3) строка в CSV: среднее и СКО времени по сериям, млн лучей/с.
// После завершения сцена продолжает вращаться; каждые 4 с переключается тепловая карта шагов.
class ExperimentRunner final {
 public:
  ExperimentRunner(Graphics& gfx, ExperimentConfig config, std::uint32_t width, std::uint32_t height);

  void RenderFrame(Graphics& gfx, FrameTimer::DeltaTime dt);

  [[nodiscard]] bool IsFinished() const noexcept;

 private:
  struct SceneCase {
    SceneKind kind;
    std::uint32_t resolution;         // 0 для модели — берётся из файла
    std::filesystem::path model_path;
  };

  enum class Phase { kPrepare, kWarmup, kMeasure, kFinished };

  [[nodiscard]] bool LoadScene();
  void PrepareRepresentation(Graphics& gfx);
  void PrepareView(Graphics& gfx);
  void FinishView(Graphics& gfx);
  void AdvanceRepresentation(Graphics& gfx);
  void WriteCpuOnlyRows(std::string const& gpu_status);
  void RenderIdle(Graphics& gfx, FrameTimer::DeltaTime dt);
  [[nodiscard]] MeasurementRow MakeRow(std::size_t view) const;
  [[nodiscard]] CameraBasis ViewBasis(std::size_t view) const noexcept;

  ExperimentConfig config_;
  std::uint32_t width_;
  std::uint32_t height_;
  CsvReport report_;
  profiling::GpuTimer timer_;
  std::vector<SceneCase> cases_;

  Phase phase_ = Phase::kPrepare;
  std::size_t case_index_ = 0u;
  std::size_t representation_index_ = 0u;
  std::size_t view_index_ = 0u;

  bool scene_loaded_ = false;
  std::uint32_t resolution_ = 0u;
  std::vector<std::uint64_t> codes_;
  double generation_ms_ = 0.0;

  std::unique_ptr<IVoxelGrid> grid_;
  std::unique_ptr<drawable::VoxelVolume> volume_;
  double build_ms_ = 0.0;

  ImageTraceStats cpu_stats_{};
  std::uint32_t frame_in_phase_ = 0u;
  std::uint32_t series_done_ = 0u;
  std::vector<double> frame_ms_;
  std::vector<double> series_ms_;

  float idle_time_ = 0.f;
};
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_EXPERIMENT_RUNNER_HPP
