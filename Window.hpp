#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "OptimisedWindowsHeader.hpp"
#include "WindowClass.hpp"

namespace gpu_renderer {
class Window {
 public:
  static constexpr DWORD kNoExtraStyle{NULL};

 private:
  static constexpr HWND kNoParent{NULL};
  static constexpr HMENU kNoMenu{NULL};

 public:
  Window() = delete;

  Window(WindowClass const& window_class, LPCWSTR lpszWindowName, DWORD dwStyle,
         int x, int y, int nWidth, int nHeight, HINSTANCE hInstance,
         DWORD dwExStyle) noexcept;
  Window(Window const&) = delete;
  Window(Window&&) noexcept = delete;

  Window& operator=(Window const&) = delete;
  Window& operator=(Window&&) = delete;

  virtual ~Window() noexcept;

  void Show() const noexcept;
  void Show(int nCmdShow) const noexcept;
  void Hide() const noexcept;

  static WNDPROC GetlpfnWndProc() noexcept;

 private:
  static LRESULT WINAPI SetupWindowProcW(_In_ HWND hWnd, _In_ UINT Msg,
                                         _In_ WPARAM wParam,
                                         _In_ LPARAM lParam) noexcept;
  static LRESULT WINAPI DisptachWindowProcW(_In_ HWND hWnd, _In_ UINT Msg,
                                            _In_ WPARAM wParam,
                                            _In_ LPARAM lParam) noexcept;

 protected:
  virtual LRESULT HandleWinMessage(UINT Msg, WPARAM wParam, LPARAM lParam);

 private:
  HWND hwnd_{NULL};

  static bool first_show_done_;
};
}  // namespace gpu_renderer

#endif  // !WINDOW_HPP