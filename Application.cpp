#include "Application.hpp"

namespace gpu_renderer {
Application::Application(HINSTANCE hInstance, int nCmdShow)
    : window_class_{hInstance, window::Canvas::GetlpfnWndProc()},
      window_{window_class_, kName,   kLeftTopCornerPosX, kLeftTopCornerPosY,
              kWidth,        kHeight, hInstance} {
  window_.Show(nCmdShow);
}

int Application::Run() const {
  return window_.LockInMessageQueue();
}
}  // namespace gpu_renderer
