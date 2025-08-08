#include "Application.hpp"

namespace gpu_renderer {
Application::Application(HINSTANCE hInstance, int nCmdShow)
    : window_class_{hInstance, window::Canvas::GetlpfnWndProc()},
      window_{window_class_, kName,   kLeftTopCornerPosX, kLeftTopCornerPosY,
              kWidth,        kHeight, hInstance} {
  window_.Show(nCmdShow);
}

int Application::Run() {
  while (!exit_code_) {
    Process();
    Update();
    Render();
  }
  return *exit_code_;
}
void Application::Process() { exit_code_ = window_.ProcessMessagesFromQueue(); }
void Application::Update() {}
void Application::Render() {}
}  // namespace gpu_renderer
