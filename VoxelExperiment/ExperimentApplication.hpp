#ifndef VOXEL_EXPERIMENT_APPLICATION_HPP
#define VOXEL_EXPERIMENT_APPLICATION_HPP

#include "../Canvas.hpp"
#include "../FrameTimer.hpp"
#include "ExperimentConfig.hpp"
#include "ExperimentRunner.hpp"

namespace gpu_renderer::voxel {
// Приложение вычислительного эксперимента: то же окно и цикл сообщений, что у Application,
// но вместо демонстрационных кубов кадры рисует ExperimentRunner.
// Запуск: GPU-Renderer.exe --voxel-experiment [опции, см. ExperimentConfig.hpp]
class ExperimentApplication final {
 private:
  // Graphics задаёт область вывода 640x480 — размеры должны совпадать.
  static constexpr int kWidth = 1920;
  static constexpr int kHeight = 1080;
  static constexpr int kLeftTopCornerPosX = 15;
  static constexpr int kLeftTopCornerPosY = 30;
  static constexpr LPCWSTR kName = L"GPU-Renderer: voxel experiment";

 public:
  ExperimentApplication(HINSTANCE hInstance, int nCmdShow, ExperimentConfig config);

  window::ExitCode Run();

 private:
  std::optional<window::ExitCode> Process() noexcept;

 private:
  window::CachedDC window_class_;
  window::Canvas wnd_;
  ExperimentRunner runner_;
};
}  // namespace gpu_renderer::voxel

#endif  // !VOXEL_EXPERIMENT_APPLICATION_HPP
