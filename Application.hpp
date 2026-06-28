#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Canvas.hpp"
#include "FrameTimer.hpp"
#include "Drawable.hpp"

namespace gpu_renderer {
class Application final {
 private:
  static constexpr int kWidth = 640;
  static constexpr int kHeight = 480;
  static constexpr int kLeftTopCornerPosX = 15;
  static constexpr int kLeftTopCornerPosY = 30;
  static constexpr LPCWSTR kName = L"GPU-Renderer";

 public:
  Application(HINSTANCE hInstance, int nCmdShow);

  window::ExitCode Run();

 private:
  std::optional<window::ExitCode> Process() noexcept;
  void Update(FrameTimer::DeltaTime dt);
  void Render();

 private:
  window::CachedDC window_class_;
  window::Canvas wnd_;

  std::vector<std::unique_ptr<Drawable>> drawable_objs_;
};
}  // namespace gpu_renderer

#endif  // !APPLICATION_HPP
