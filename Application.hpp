#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Canvas.hpp"
#include "FrameTimer.hpp"

namespace gpu_renderer {
class Application final {
 private:
  static constexpr int kWidth{640};
  static constexpr int kHeight{480};
  static constexpr int kLeftTopCornerPosX{15};
  static constexpr int kLeftTopCornerPosY{30};
  static constexpr LPCWSTR kName{L"GPU-Renderer"};

 public:
  Application() = delete;
  Application(HINSTANCE hInstance, int nCmdShow);
  Application(Application const&) = delete;
  Application(Application&&) = delete;

  Application& operator=(Application const&) = delete;
  Application& operator=(Application&&) = delete;

  ~Application() = default;

  int Run();
 
 private:
  void Process();
  void Update(FrameTimer::DeltaTime dt);
  void Render();

 private:
  window::CachedDC window_class_;
  window::Canvas window_;

  std::optional<window::ExitCode> exit_code_{};
};
}  // namespace gpu_renderer

#endif  // !APPLICATION_HPP
