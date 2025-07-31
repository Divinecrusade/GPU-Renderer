#include "Canvas.hpp"

gpu_renderer::Canvas::Canvas(CachedDC& window_class, LPCWSTR lpszWindowName,
                             POINT left_top_corner_pos, SIZE window_dimension,
                             HINSTANCE hInstance) noexcept
    : Window{window_class,
             lpszWindowName,
             kFrameWithMinimize,
             static_cast<int>(left_top_corner_pos.x),
             static_cast<int>(left_top_corner_pos.y),
             static_cast<int>(window_dimension.cx),
             static_cast<int>(window_dimension.cy),
             hInstance,
             Window::kNoExtraStyle} 
{}
