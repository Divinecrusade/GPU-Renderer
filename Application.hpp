#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Canvas.hpp"

namespace gpu_renderer {
class Application final {
 public:
  Application(HINSTANCE hInstance, int nCmdShow);

  int Run() const;

 private:
  static constexpr int kWidth{640};
  static constexpr int kHeight{480};
  static constexpr int kLeftTopCornerPosX{15};
  static constexpr int kLeftTopCornerPosY{30};
  static constexpr LPCWSTR kName{L"GPU-Renderer"};

 private:
  window::CachedDC window_class_;
  window::Canvas window_;
};
}  // namespace gpu_renderer

#endif  // !APPLICATION_HPP
