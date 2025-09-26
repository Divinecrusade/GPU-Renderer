#include "Application.hpp"
#include "FrameTimer.hpp"

namespace gpu_renderer {
Application::Application(HINSTANCE hInstance, int nCmdShow)
    : window_class_{hInstance, window::Canvas::GetlpfnWndProc()},
      window_{window_class_, kName,   kLeftTopCornerPosX, kLeftTopCornerPosY,
              kWidth,        kHeight, hInstance} {
  window_.Show(nCmdShow);
}

window::ExitCode Application::Run() {
  std::optional<window::ExitCode> exit_code = Process();

  for (FrameTimer ft{}; !exit_code; exit_code = Process()) {
    Update(ft.Mark());
    Render();
  }

  return *exit_code;
}

std::optional<window::ExitCode> Application::Process() {
  constexpr bool kLockInQueue = false;
  return window_.ProcessMessagesFromQueue<kLockInQueue>();
}

void Application::Update(FrameTimer::DeltaTime dt) {
}

void Application::Render() { 
  window_.gfx.ClearBuffer({0.f, 0.f, 0.f});
  window_.gfx.DrawTestTriangle();
  window_.gfx.EndFrame(); 
}
}  // namespace gpu_renderer
