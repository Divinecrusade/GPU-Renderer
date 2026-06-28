#include "Application.hpp"
#include "Box.hpp"
#include "FrameTimer.hpp"

namespace gpu_renderer {
Application::Application(HINSTANCE hInstance, int nCmdShow)
    : window_class_{hInstance, window::Canvas::GetlpfnWndProc()},
      wnd_{window_class_, kName,   kLeftTopCornerPosX, kLeftTopCornerPosY,
              kWidth,        kHeight, hInstance} {
  wnd_.Show(nCmdShow);
  wnd_.gfx.SetProjection(DirectX::XMMatrixPerspectiveLH(1.f, 3.f / 4.f, 0.2f, 40.f));

  std::mt19937 rng{std::random_device{}()};
  std::uniform_real_distribution<float> angle_dist{0.f, std::numbers::pi_v<float> * 2.f};
  std::uniform_real_distribution<float> pos_dist{0.f, std::numbers::pi_v<float> * 2.f};
  std::uniform_real_distribution<float> angle_delta_dist{0.f, std::numbers::pi_v<float> * 0.3f};
  std::uniform_real_distribution<float> radius_dist{6.f, 20.f};
  constexpr std::size_t kCubesCount = 80u;
  drawable_objs_.reserve(kCubesCount);
  for (std::size_t i = 0; i < kCubesCount; ++i) {
    drawable_objs_.push_back(std::make_unique<drawable::Box>(wnd_.gfx, rng, angle_dist, pos_dist, angle_delta_dist, radius_dist));
  }
}

window::ExitCode Application::Run() {
  std::optional<window::ExitCode> exit_code = Process();

  for (FrameTimer ft{}; !exit_code; exit_code = Process()) {
    Update(ft.Mark());
    Render();
  }

  return *exit_code;
}

std::optional<window::ExitCode> Application::Process() noexcept {
  constexpr bool kLockInQueue = false;
  return wnd_.ProcessMessagesFromQueue<kLockInQueue>();
}

void Application::Update(FrameTimer::DeltaTime dt) {
  for (auto& drawable_obj : drawable_objs_) {
    drawable_obj->Update(dt);
  }
}

void Application::Render() {
  wnd_.gfx.ClearBuffer({0.f, 0.f, 0.f});
  for (auto& drawable_obj : drawable_objs_) {
    drawable_obj->Draw(wnd_.gfx);
  }
  wnd_.gfx.EndFrame();
}
}  // namespace gpu_renderer
