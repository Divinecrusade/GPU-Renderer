#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <gsl/gsl>

#include "Keyboard.hpp"
#include "Mouse.hpp"
#include "OptimisedWindowsHeader.hpp"
#include "WindowClass.hpp"
#include "WinError.hpp"

namespace gpu_renderer::window {
using namespace input;
using ExitCode = int;

class Window {
 public:
  static constexpr DWORD kNoExtraStyle{NULL};

 private:
  static constexpr HWND kNoParent{NULL};
  static constexpr HMENU kNoMenu{NULL};

  static constexpr HWND kAllWindows{NULL};
  static constexpr UINT kNoMinRangeFilterMsg{NULL};
  static constexpr UINT kNoMaxRangeFilterMsg{NULL};
  static constexpr auto ErrorHappened = [](BOOL result) {
    return result == -1;
  };

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

  template<bool kTranslateMessages = true>
  static int LockInMessageQueue() {
    MSG msg{};
    while (auto const operation_done{GetMessageW(
        &msg, kAllWindows, kNoMinRangeFilterMsg, kNoMaxRangeFilterMsg)}) {
      if (ErrorHappened(operation_done)) {
#ifdef _DEBUG
        throw exception::WinError{__FILEW__, __LINE__,
                                  "Handling message provokes error",
                                  GetLastError()};
#else
        throw exception::WinError{"Event goes wrong", GetLastError()};
#endif  // _DEBUG
      } else {
        if constexpr (kTranslateMessages) {
          std::ignore = TranslateMessage(&msg);
        }
        std::ignore = DispatchMessageW(&msg);
      }
    }

    return gsl::narrow<ExitCode>(msg.wParam);
  }

  template <bool kTranslateMessages = true>
  static std::optional<ExitCode> ProcessMessagesFromQueue() {
    static MSG msg{};

    while (PeekMessageW(&msg, kAllWindows, kNoMinRangeFilterMsg,
                        kNoMaxRangeFilterMsg, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        return gsl::narrow<ExitCode>(msg.wParam);
      }

      if constexpr (kTranslateMessages) {
        std::ignore = TranslateMessage(&msg);
      }
      std::ignore = DispatchMessageW(&msg);
    }
    return std::nullopt;
  }

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
}  // namespace gpu_renderer::window

#endif  // !WINDOW_HPP