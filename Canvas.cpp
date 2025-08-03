#include "Canvas.hpp"

gpu_renderer::Canvas::Canvas(CachedDC& window_class, LPCWSTR lpszWindowName,
                             int left_top_pos_x, int left_top_pos_y, 
                             int width, int height,
                             HINSTANCE hInstance) noexcept
    : Window{window_class,
             lpszWindowName,
             kFrameWithMinimize,
             left_top_pos_x,
             left_top_pos_y,
             width,
             height,
             hInstance,
             Window::kNoExtraStyle} 
{}
