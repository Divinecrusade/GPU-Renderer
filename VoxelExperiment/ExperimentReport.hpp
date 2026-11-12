#ifndef VOXEL_EXPERIMENT_REPORT_HPP
#define VOXEL_EXPERIMENT_REPORT_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>

// Запись результатов эксперимента в CSV (разделитель «;» — удобно открывать в Excel
// с русской локалью) и статистика: среднее, выборочное СКО, показатель степенного роста.
namespace gpu_renderer::voxel {
struct MeasurementRow {
  std::string scene;
  std::uint32_t resolution = 0u;
  std::string representation;
  std::string traversal = "-";  // stack / restart для SVO и SVDAG, "-" для остальных
  std::size_t view = 0u;
  std::size_t boundary_voxels = 0u;
  double generation_ms = 0.0;
  double build_ms = 0.0;
  std::size_t memory_bytes = 0u;
  std::size_t cpu_rays = 0u;  // число лучей CPU-обхода (зависит от --voxel-cpu-stride)
  double cpu_mean_steps = 0.0;
  double cpu_mean_fetches = 0.0;  // чтений 32-битных слов буфера на луч
  double hit_rate = 0.0;
  std::string gpu_status;  // "ok", "skipped: ...", "failed: ..."
  double gpu_ms_mean = 0.0;
  double gpu_ms_std = 0.0;
  double gpu_ms_min = 0.0;  // самый быстрый кадр за все серии: устойчив к помехам от других процессов
  double mrays_per_second = 0.0;
};

class CsvReport final {
 public:
  explicit CsvReport(std::filesystem::path const& path);

  void Append(MeasurementRow const& row);

 private:
  std::ofstream file_;
};

[[nodiscard]] double Mean(std::span<double const> values) noexcept;
// Выборочное стандартное отклонение (n - 1 в знаменателе).
[[nodiscard]] double StdDev(std::span<double const> values) noexcept;
// Показатель alpha в y = C * x^alpha методом наименьших квадратов по (ln x, ln y).
[[nodiscard]] double FitPowerLawExponent(std::span<double const> x, std::span<double const> y) noexcept;
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_EXPERIMENT_REPORT_HPP
