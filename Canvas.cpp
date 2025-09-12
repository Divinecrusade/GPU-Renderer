#include "Canvas.hpp"

namespace gpu_renderer::window {
Canvas::Canvas(CachedDC& window_class, LPCWSTR lpszWindowName,
               int left_top_pos_x, int left_top_pos_y, int width, int height,
               HINSTANCE hInstance)
    : Window{Keyboard::kDefaultEventsQueueSize,
             0u,
             Mouse::kDefaultEventsQueueSize,
             window_class,
             lpszWindowName,
             kFrameWithMinimize,
             left_top_pos_x,
             left_top_pos_y,
             width,
             height,
             hInstance,
             Window::kNoExtraStyle},
      gfx{Window::GetHWND()}
{}

LRESULT Canvas::HandleMessage(UINT Msg, WPARAM wParam, LPARAM lParam) noexcept {
  if (Msg == WM_CHAR) [[unlikely]] {
    return DefWindowProcW(GetHWND(), Msg, wParam, lParam);
  } else [[likely]] {
    return Window::HandleMessage(Msg, wParam, lParam);
  }
}
}  // namespace gpu_renderer::window
