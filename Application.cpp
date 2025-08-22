#include "Application.hpp"
#include "FrameTimer.hpp"

namespace gpu_renderer {
Application::Application(HINSTANCE hInstance, int nCmdShow)
    : window_class_{hInstance, window::Canvas::GetlpfnWndProc()},
      window_{window_class_, kName,   kLeftTopCornerPosX, kLeftTopCornerPosY,
              kWidth,        kHeight, hInstance} {
  window_.Show(nCmdShow);
  Process();
}

int Application::Run() {
  for (FrameTimer ft{}; !exit_code_; Process()) {
    Update(ft.Mark());
    Render();
  }
  return *exit_code_;
}

void Application::Process() { exit_code_ = window_.ProcessMessagesFromQueue<false>(); }

void Application::Update(FrameTimer::DeltaTime dt) {
  constexpr float kDeltaTheta{3.14f / 4.f};
  theta = std::fmodf(theta + kDeltaTheta * dt, 3.14f);
}

void Application::Render() { 
  window_.gfx.ClearBuffer({std::sinf(theta), 1.f, 1.f});
  window_.gfx.EndFrame(); 
}
}  // namespace gpu_renderer
