#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "Keyboard.hpp"
#include "Mouse.hpp"
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
         DWORD dwExStyle);
  Window(Window const&) = delete;
  Window(Window&&) noexcept = delete;

  Window& operator=(Window const&) = delete;
  Window& operator=(Window&&) = delete;

  virtual ~Window() noexcept;

  void Show() const noexcept;
  void Show(int nCmdShow) const noexcept;
  void Hide() const noexcept;

  void Enable() const noexcept;
  void Disable() const noexcept;

  [[nodiscard]] bool IsEnabled() const noexcept;
  [[nodiscard]] bool IsShown() const noexcept;

  [[nodiscard]] Keyboard::View GetKeyboard() noexcept;
  [[nodiscard]] Mouse::View GetMouse() noexcept;

  [[nodiscard]] static WNDPROC GetlpfnWndProc() noexcept;

 protected:
  Window(std::size_t keyboard_events_queue_size,
         std::size_t keyboard_chars_buffer_size,
         std::size_t mouse_events_queue_size, WindowClass const& window_class,
         LPCWSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth,
         int nHeight, HINSTANCE hInstance, DWORD dwExStyle);

  virtual LRESULT HandleMessage(UINT Msg, WPARAM wParam,
                                LPARAM lParam) noexcept;

  [[nodiscard]] HWND GetHWND() const noexcept;

  static void IncreaseCounterOfActiveWindows(unsigned delta = 1u) noexcept;
  static void DecreaseCounterOfActiveWindows(unsigned delta = 1u) noexcept;
  [[nodiscard]] static unsigned GetCountOfActiveWindows() noexcept;

 private:
  static HWND InitializeWindow(Window* window_instance, LPCWSTR lpClassName,
                               LPCWSTR lpszWindowName, DWORD dwStyle, int x,
                               int y, int nWidth, int nHeight,
                               HINSTANCE hInstance, DWORD dwExStyle);

  static LRESULT WINAPI SetupWindowProcW(_In_ HWND hWnd, _In_ UINT Msg,
                                         _In_ WPARAM wParam,
                                         _In_ LPARAM lParam) noexcept;
  static LRESULT WINAPI DisptachWindowProcW(_In_ HWND hWnd, _In_ UINT Msg,
                                            _In_ WPARAM wParam,
                                            _In_ LPARAM lParam) noexcept;

 private:
  int width_;
  int height_;

  Keyboard kbd_{Keyboard::kDefaultEventsQueueSize, 
                Keyboard::kDefaultCharsBufferSize};
  Mouse mse_{Mouse::kDefaultEventsQueueSize};

  HWND hwnd_{NULL};

  static unsigned active_windows_count_;
#ifdef _DEBUG
  static bool first_show_done_;
#endif  // _DEBUG
};
}  // namespace gpu_renderer

#endif  // !WINDOW_HPP