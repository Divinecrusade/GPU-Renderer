#ifndef WINDOW_CLASS_HPP
#define WINDOW_CLASS_HPP

#include "OptimisedWindowsHeader.hpp"

namespace gpu_renderer {
class WindowClass {
 public:
  static constexpr HICON DEFAULT_ICON{NULL};
  static constexpr HCURSOR DEFAULT_CURSOR{NULL};
  static constexpr HBRUSH NO_BACKGROUND{NULL};
  static constexpr LPCWSTR NO_MENU_NAME{NULL};

 public:
  WindowClass() = delete;
  WindowClass(UINT style, WNDPROC lpfnWndProc, int cbClsExtra, int cbWndExtra,
              HINSTANCE hInstance, HICON hIcon, HCURSOR hCursor,
              HBRUSH hbrBackground, LPCWSTR lpszMenuName,
              LPCWSTR lpszClassName, HICON hIconSm) noexcept;
  WindowClass(WindowClass&) = delete;
  WindowClass(WindowClass&&) noexcept = default;

  WindowClass& operator=(WindowClass const&) = delete;
  WindowClass& operator=(WindowClass&&) = delete;

  virtual ~WindowClass() noexcept;
 
 private:
  HINSTANCE hInstance;
  LPCWSTR lpClassName;
};
} // namespace gpu_renderer

#endif  // !WINDOW_CLASS_HPP
