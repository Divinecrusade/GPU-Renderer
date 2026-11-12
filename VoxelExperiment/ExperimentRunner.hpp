#ifndef VOXEL_EXPERIMENT_RUNNER_HPP
#define VOXEL_EXPERIMENT_RUNNER_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../FrameTimer.hpp"
#include "../Graphics.hpp"
#include "ExperimentConfig.hpp"
#include "ExperimentReport.hpp"
#include "GpuTimer.hpp"
#include "RayTraversal.hpp"
#include "SceneViewer.hpp"
#include "VoxelGrid.hpp"
#include "VoxelVolume.hpp"

namespace gpu_renderer::voxel {
// Автомат эксперимента, выполняемый по одному кадру за вызов RenderFrame.
// Для каждой сцены (сцена x N или модель) и каждого варианта (представление; для SVO и SVDAG —
// также способ спуска, см. BuildVariants):
//   1) построение структуры на CPU (время, объём памяти);
//   2) для каждого ракурса — CPU-обход того же кадра с шагом cpu_stride (среднее число шагов
//      на луч, доля попаданий), затем на GPU warmup кадров и series x frames кадров с замером
//      времени по timestamp-запросам;
//   3) строка в CSV: среднее и СКО времени по сериям, млн лучей/с.
// После завершения (или сразу, при view_only) управление передаётся SceneViewer:
// сцены переключаются клавишами, сгенерированные сцены берутся из кэша.
class ExperimentRunner final {
 public:
  ExperimentRunner(Graphics& gfx, ExperimentConfig config, std::uint32_t width, std::uint32_t height);

  void RenderFrame(Graphics& gfx, FrameTimer::DeltaTime dt);

  // Нажатия клавиш; во время замеров игнорируются, чтобы не влиять на результаты.
  void OnKeyPressed(std::uint8_t key) noexcept;

  [[nodiscard]] bool IsFinished() const noexcept;

  // Текст для заголовка окна, если он изменился с прошлого вызова.
  [[nodiscard]] std::optional<std::wstring> TakeStatusChange();

 private:
  enum class Phase { kPrepare, kWarmup, kMeasure, kFinished };

  [[nodiscard]] bool LoadScene();
  void PrepareRepresentation(Graphics& gfx);
  void PrepareView(Graphics& gfx);
  void FinishView(Graphics& gfx);
  void AdvanceRepresentation();
  void StartViewer();
  void WriteCpuOnlyRows(std::string const& gpu_status);
  [[nodiscard]] MeasurementRow MakeRow(std::size_t view) const;
  [[nodiscard]] CameraBasis ViewBasis(std::size_t view) const noexcept;
  [[nodiscard]] std::wstring ProgressText() const;

  ExperimentConfig config_;
  std::vector<MeasuredVariant> variants_;
  std::uint32_t width_;
  std::uint32_t height_;
  std::optional<CsvReport> report_;  // не создаётся при view_only, чтобы не затереть прошлый отчёт
  profiling::GpuTimer timer_;
  std::vector<SceneCase> cases_;
  std::vector<std::optional<LoadedScene>> cache_;  // сцены для просмотра после замеров

  Phase phase_ = Phase::kPrepare;
  std::size_t case_index_ = 0u;
  std::size_t representation_index_ = 0u;
  std::size_t view_index_ = 0u;

  bool scene_loaded_ = false;
  LoadedScene scene_{};

  std::unique_ptr<IVoxelGrid> grid_;
  std::unique_ptr<drawable::VoxelVolume> volume_;
  double build_ms_ = 0.0;
  bool reuse_grid_ = false;               // следующий вариант — то же представление с другим спуском
  std::size_t skipped_memory_bytes_ = 0u;  // объём плотной сетки, не построенной из-за размера

  ImageTraceStats cpu_stats_{};
  std::uint32_t frame_in_phase_ = 0u;
  std::uint32_t series_done_ = 0u;
  std::vector<double> frame_ms_;
  std::vector<double> series_ms_;
  double min_frame_ms_ = 0.0;  // минимум по всем кадрам ракурса; 0 — кадров ещё нет

  std::unique_ptr<SceneViewer> viewer_;
  std::wstring shown_status_;
};
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_EXPERIMENT_RUNNER_HPP
