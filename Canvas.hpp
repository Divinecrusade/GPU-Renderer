#ifndef CANVAS_HPP
#define CANVAS_HPP

#include "CachedDC.hpp"
#include "Graphics.hpp"
#include "Window.hpp"

namespace gpu_renderer::window {
class Canvas : public Window {
 public:
  Canvas(CachedDC& window_class, LPCWSTR lpszWindowName,
         int left_top_pos_x, int left_top_pos_y,
         int width, int height, HINSTANCE hInstance);

 protected:
  LRESULT HandleMessage(UINT Msg, WPARAM wParam, LPARAM lParam) noexcept override;

 public:
  Graphics gfx;

 private:
  static constexpr DWORD kFrameWithMinimize= WS_MINIMIZEBOX |
                                             WS_CAPTION |
                                             WS_SYSMENU;
};
}  // namespace gpu_renderer

#endif  // !CANVAS_HPP
