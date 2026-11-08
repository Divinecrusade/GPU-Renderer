#include "ExperimentApplication.hpp"

namespace gpu_renderer::voxel {
ExperimentApplication::ExperimentApplication(HINSTANCE hInstance, int nCmdShow, ExperimentConfig config)
    : window_class_{hInstance, window::Canvas::GetlpfnWndProc()},
      wnd_{window_class_, kName, kLeftTopCornerPosX, kLeftTopCornerPosY, kWidth, kHeight, hInstance},
      runner_{wnd_.gfx, std::move(config), static_cast<std::uint32_t>(kWidth), static_cast<std::uint32_t>(kHeight)} {
  wnd_.Show(nCmdShow);
}

window::ExitCode ExperimentApplication::Run() {
  std::optional<window::ExitCode> exit_code = Process();

  for (FrameTimer ft{}; !exit_code; exit_code = Process()) {
    for (auto keyboard = wnd_.GetKeyboard(); auto const key_event = keyboard.GetOldestEvent();) {
      if (key_event->second == input::Keyboard::EventType::kPressed) {
        runner_.OnKeyPressed(key_event->first);
      }
    }
    runner_.RenderFrame(wnd_.gfx, ft.Mark());
    if (auto const status = runner_.TakeStatusChange()) {
      wnd_.SetTitle(std::wstring{kName} + L" — " + *status);
    }
  }

  return *exit_code;
}

std::optional<window::ExitCode> ExperimentApplication::Process() noexcept {
  constexpr bool kLockInQueue = false;
  return wnd_.ProcessMessagesFromQueue<kLockInQueue>();
}
}  // namespace gpu_renderer::voxel
