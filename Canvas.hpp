#ifndef CANVAS_HPP
#define CANVAS_HPP

#include "CachedDC.hpp"
#include "DebugHeader.hpp"
#include "OptimisedWindowsHeader.hpp"
#include "Window.hpp"

namespace gpu_renderer {
class Canvas : public Window {
 private:
  static constexpr POINT kLeftTopWithPadding{10, 30};
  static constexpr DWORD kFrameWithMinimize{WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU};

 public:
  Canvas() = delete;
  Canvas(CachedDC& window_class, LPCWSTR lpszWindowName, int nWidth,
         int nHeight, HINSTANCE hInstance) noexcept;
  Canvas(Canvas const&) = delete;
  Canvas(Canvas&&) = delete;

  Canvas& operator=(Canvas const&) = delete;
  Canvas& operator=(Canvas&&) = delete;

  ~Canvas() = default;
};
}  // namespace gpu_renderer

#endif  // !CANVAS_HPP
