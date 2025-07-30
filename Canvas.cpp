#include "Canvas.hpp"

gpu_renderer::Canvas::Canvas(CachedDC& window_class, LPCWSTR lpszWindowName,
                             int nWidth, int nHeight,
                             HINSTANCE hInstance) noexcept
    : Window{window_class,
             lpszWindowName,
             kFrameWithMinimize,
             kLeftTopWithPadding.x,
             kLeftTopWithPadding.y,
             nWidth,
             nHeight,
             hInstance,
             Window::kNoExtraStyle} 
{}
