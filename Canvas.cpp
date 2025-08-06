#include "Canvas.hpp"

gpu_renderer::Canvas::Canvas(CachedDC& window_class, LPCWSTR lpszWindowName,
                             int left_top_pos_x, int left_top_pos_y, 
                             int width, int height,
                             HINSTANCE hInstance)
    : Window{Keyboard::kDefaultEventsQueueSize,
             0ull,
             Mouse::kDefaultEventsQueueSize,
             window_class,
             lpszWindowName,
             kFrameWithMinimize,
             left_top_pos_x,
             left_top_pos_y,
             width,
             height,
             hInstance,
             Window::kNoExtraStyle} 
{}

LRESULT gpu_renderer::Canvas::HandleMessage(UINT Msg, WPARAM wParam,
                                            LPARAM lParam) noexcept {
  if (Msg == WM_CHAR) {
    return DefWindowProcW(GetHWND(), Msg, wParam, lParam);
  } else {
    return Window::HandleMessage(Msg, wParam, lParam);
  }
}
