#include "ExperimentReport.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <stdexcept>

namespace gpu_renderer::voxel {
CsvReport::CsvReport(std::filesystem::path const& path) : file_{path, std::ios::out | std::ios::trunc} {
  if (!file_) {
    throw std::runtime_error{"Cannot create report file: " + path.string()};
  }
  file_ << "scene;N;representation;view;boundary_voxels;generation_ms;build_ms;memory_bytes;"
           "cpu_mean_steps;hit_rate;gpu_status;gpu_ms_mean;gpu_ms_std;mrays_per_s\n";
}

void CsvReport::Append(MeasurementRow const& row) {
  file_ << std::fixed << std::setprecision(4) << row.scene << ';' << row.resolution << ';' << row.representation
        << ';' << row.view << ';' << row.boundary_voxels << ';' << row.generation_ms << ';' << row.build_ms << ';'
        << row.memory_bytes << ';' << row.cpu_mean_steps << ';' << row.hit_rate << ';' << row.gpu_status << ';'
        << row.gpu_ms_mean << ';' << row.gpu_ms_std << ';' << row.mrays_per_second << '\n';
  file_.flush();  // результаты не теряются, если прогон прервётся
}

double Mean(std::span<double const> values) noexcept {
  if (values.empty()) {
    return 0.0;
  }
  double sum = 0.0;
  for (double const v : values) {
    sum += v;
  }
  return sum / static_cast<double>(values.size());
}

double StdDev(std::span<double const> values) noexcept {
  if (values.size() < 2u) {
    return 0.0;
  }
  double const mean = Mean(values);
  double sum = 0.0;
  for (double const v : values) {
    sum += (v - mean) * (v - mean);
  }
  return std::sqrt(sum / static_cast<double>(values.size() - 1u));
}

double FitPowerLawExponent(std::span<double const> x, std::span<double const> y) noexcept {
  std::size_t const count = std::min(x.size(), y.size());
  if (count < 2u) {
    return 0.0;
  }
  double mean_x = 0.0;
  double mean_y = 0.0;
  for (std::size_t i = 0u; i < count; ++i) {
    mean_x += std::log(x[i]);
    mean_y += std::log(y[i]);
  }
  mean_x /= static_cast<double>(count);
  mean_y /= static_cast<double>(count);
  double sxy = 0.0;
  double sxx = 0.0;
  for (std::size_t i = 0u; i < count; ++i) {
    double const dx = std::log(x[i]) - mean_x;
    sxy += dx * (std::log(y[i]) - mean_y);
    sxx += dx * dx;
  }
  return sxx > 0.0 ? sxy / sxx : 0.0;
}
}  // namespace gpu_renderer::voxel
