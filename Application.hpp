#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Canvas.hpp"

namespace gpu_renderer {
class Application final {
 private:
  static constexpr int kWidth{640};
  static constexpr int kHeight{480};
  static constexpr int kLeftTopCornerPosX{15};
  static constexpr int kLeftTopCornerPosY{30};
  static constexpr LPCWSTR kName{L"GPU-Renderer"};

 public:
  Application(HINSTANCE hInstance, int nCmdShow);

  int Run();
 
 private:
  void Process();
  void Update();
  void Render();

 private:
  window::CachedDC window_class_;
  window::Canvas window_;

  std::optional<window::ExitCode> exit_code_{};
};
}  // namespace gpu_renderer

#endif  // !APPLICATION_HPP
