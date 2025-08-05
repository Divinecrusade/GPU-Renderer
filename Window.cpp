#include "Window.hpp"
#include "WinError.hpp"

#if defined(LOG_WINDOW_MESSAGES) || defined(LOG_WINDOW)
#include <iostream>
#endif  // LOG_WINDOW_MESSAGES || LOG_WINDOW
#ifdef LOG_WINDOW_MESSAGES
#include "WinMsgFormatter.hpp"
#endif  // LOG_WINDOW_MESSAGES
#include <cassert>

unsigned gpu_renderer::Window::active_windows_count_{0u};
#ifdef _DEBUG
bool gpu_renderer::Window::first_show_done_{false};
#endif  // _DEBUG

gpu_renderer::Window::Window(WindowClass const& window_class,
                             LPCWSTR lpszWindowName, DWORD dwStyle, int x,
                             int y, int nWidth, int nHeight,
                             HINSTANCE hInstance, DWORD dwExStyle) {
  assert(((void)"Window name cannot be nullptr", lpszWindowName != nullptr));
  assert(((void)"Instance handle cannot be null", hInstance != NULL));
  assert(((void)"Width must be positive", nWidth > 0));
  assert(((void)"Height must be positive", nHeight > 0));
  assert(((void)"Current implementation of window doesn't support child window type",
          ((dwStyle & WS_CHILD) != WS_CHILD)));

  static constexpr auto WindowCreationFailed = [](HWND const& wnd) {
    return wnd == NULL;
  };

  RECT window_pos{};
  window_pos.left = x;
  window_pos.right = window_pos.left + nWidth;
  window_pos.top = y;
  window_pos.bottom = window_pos.top + nHeight;
  if (!AdjustWindowRectEx(&window_pos, dwStyle, FALSE, dwExStyle)) {
#ifdef LOG_WINDOW
    std::wcerr << L"AdjustWindowRectEx failed during Window constructing,"
               << L"error code: " << GetLastError() << "\n";
#endif  // LOG_WINDOW
  }

  hwnd_ = CreateWindowExW(dwExStyle, window_class.GetLpClassName(),
                          lpszWindowName, dwStyle, window_pos.left,
                          window_pos.top, window_pos.right - window_pos.left,
                          window_pos.bottom - window_pos.top, kNoParent,
                          kNoMenu, hInstance, reinterpret_cast<LPVOID>(this));

  if (WindowCreationFailed(hwnd_)) {
#ifdef LOG_WINDOW
    std::wcerr << L"Window creation failed\n";
    std::wcerr << L"Error code: " << GetLastError() << L"\n";
#endif  // LOG_WINDOW
#ifdef _DEBUG
    throw exception::WinError{__FILEW__, __LINE__, "CreateWindowExW returned NULL hwnd", GetLastError()};
#else
    throw exception::WinError{"It was not able to create window", GetLastError()};
#endif  // _DEBUG
  }

#ifdef LOG_WINDOW
  std::wclog << L"Window '" << lpszWindowName << L"' of class '"
             << window_class.GetLpClassName() << L"' created successfully\n";
#endif  // LOG_WINDOW
}

gpu_renderer::Window::~Window() noexcept {
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
  if (DestroyWindow(hwnd_)) {
#ifdef LOG_WINDOW
    std::wclog << L"Window destroyed successfully\n";
#endif  // LOG_WINDOW
  }
#ifdef LOG_WINDOW
  else {
    std::wcerr << L"Window destruction failed. "
               << L"Error code: " << GetLastError() << L"\n";
  }
#endif  // LOG_WINDOW
}

void gpu_renderer::Window::Show() const noexcept {
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
  assert(((void)"Window must be showed first with wWinaMain nCmdShow param",
          first_show_done_));
  (void)ShowWindow(hwnd_, SW_SHOW);
}

void gpu_renderer::Window::Show(int nCmdShow) const noexcept {
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
#ifdef _DEBUG
  if (first_show_done_) {
#ifdef LOG_WINDOW
    std::wcerr << L"Warning: This overloaded option of Show called but "
                  L"application first show already occurred\n";
#endif  // LOG_WINDOW
  }
#endif  // _DEBUG

  (void)ShowWindow(hwnd_, nCmdShow);
#ifdef _DEBUG
  first_show_done_ = true;
#endif  // _DEBUG
}

void gpu_renderer::Window::Hide() const noexcept {
  assert(((void)"Window must be showed first with wWinaMain nCmdShow param",
          first_show_done_));
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
  (void)ShowWindow(hwnd_, SW_HIDE);
}

void gpu_renderer::Window::Enable() const noexcept {
  if (BOOL const was_disabled{EnableWindow(hwnd_, TRUE)}; 
      !was_disabled) {
#ifdef LOG_WINDOW
    std::wcerr << L"Enable method called, but window has been enabled already\n";
#endif  // LOG_WINDOW
  }
}

void gpu_renderer::Window::Disable() const noexcept {
  if (BOOL const was_disabled{EnableWindow(hwnd_, FALSE)}; 
      was_disabled) {
#ifdef LOG_WINDOW
    std::wcerr
        << L"Disable method called, but window has been disabled already\n";
#endif  // LOG_WINDOW
  }
}

bool gpu_renderer::Window::IsEnabled() const noexcept {
  return IsWindowEnabled(hwnd_);
}

bool gpu_renderer::Window::IsShown() const noexcept { 
  return IsWindowVisible(hwnd_);
}

gpu_renderer::Keyboard::View gpu_renderer::Window::GetKeyboard() noexcept {
  return kbd_;
}

WNDPROC gpu_renderer::Window::GetlpfnWndProc() noexcept {
  return SetupWindowProcW;
}

LRESULT WINAPI gpu_renderer::Window::SetupWindowProcW(
    _In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam,
    _In_ LPARAM lParam) noexcept {
  if (Msg == WM_CREATE) {
#ifdef LOG_WINDOW_MESSAGES
    std::wclog << L"Installing wndproc for new instance of Window...\n";
    std::wclog << WinMsgFormatter{}(Msg, wParam, lParam) << "\n";
#endif  // LOG_WINDOW_MESSAGES
    CREATESTRUCTW const* const creation_params{
        reinterpret_cast<CREATESTRUCTW const*>(lParam)};
    assert(((void)"lParam must be a pointer to window creation struct",
            creation_params != nullptr));
    Window* const window_instance{
        reinterpret_cast<Window*>(creation_params->lpCreateParams)};
    assert(((void)"Window creation struct must have the pointer to Window "
                  "class object",
            window_instance != nullptr));
    (void)SetWindowLongPtrW(hWnd, GWLP_USERDATA,
                            reinterpret_cast<LONG_PTR>(window_instance));
    (void)SetWindowLongPtrW(hWnd, GWLP_WNDPROC,
                            reinterpret_cast<LONG_PTR>(DisptachWindowProcW));
    ++active_windows_count_;
#ifdef LOG_WINDOW
    std::wclog << L"Done installing wndproc for new instance of Window\n";
    std::wclog << L"There're " << active_windows_count_
               << L" active windows right now\n";
#endif  // LOG_WINDOW
  }
  return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

LRESULT WINAPI gpu_renderer::Window::DisptachWindowProcW(
    _In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam,
    _In_ LPARAM lParam) noexcept {
#ifdef LOG_WINDOW_MESSAGES
  std::wclog << L"Dispatcher get windows message...\n";
  std::wclog << WinMsgFormatter{}(Msg, wParam, lParam) << "\n";
#endif  // LOG_WINDOW_MESSAGES
  Window* const window_instance{
      reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA))};
  assert(((void)"Window data must have the pointer to Window class object",
          window_instance != nullptr));
#ifdef LOG_WINDOW_MESSAGES
  std::wclog << L"...forward this message to Window object\n";
#endif  // LOG_WINDOW_MESSAGES
  return window_instance->HandleMessage(Msg, wParam, lParam);
}

LRESULT gpu_renderer::Window::HandleMessage(UINT Msg, WPARAM wParam,
                                               LPARAM lParam) noexcept {
  static constexpr auto kPreviousKeyStateMask{0b1000000000000000000000000000000};
#ifdef LOG_WINDOW_MESSAGES
  std::wclog << L"Message catched in Window object\n";
#endif  // LOG_WINDOW_MESSAGES
  switch (Msg) {
    case WM_CLOSE: {
      assert(((void)"Counter of windows cannot be equal to zero "
                    "during closing of window",
              active_windows_count_ != 0u));
      --active_windows_count_;
      Disable();
      Hide();
#ifdef LOG_WINDOW
      std::wclog << L"Window disabled and hidden (inactive). There're " 
                 << active_windows_count_
                 << L" active windows right now\n";
#endif  // LOG_WINDOW
      if (active_windows_count_ == 0u) {
#ifdef LOG_WINDOW
        std::wclog << L"No active windows, posting quit message queue...\n";
#endif  // LOG_WINDOW
        PostQuitMessage(EXIT_SUCCESS);   
      }
      return 0;
    }
    case WM_KILLFOCUS: {
      kbd_.ClearKeysState();
      kbd_.ClearKeyEventsQueue();
    } break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
      if (!(lParam & kPreviousKeyStateMask) || kbd_.IsAutoRepeating()) {
        kbd_.OnKeyDown(static_cast<unsigned char>(wParam));
      }
    } break;
    case WM_KEYUP: {
      kbd_.OnKeyUp(static_cast<unsigned char>(wParam));
    } break;
    case WM_CHAR: {
      kbd_.OnChar(static_cast<wchar_t>(wParam));
    } break;
  }
  return DefWindowProcW(hwnd_, Msg, wParam, lParam);
}

void gpu_renderer::Window::IncreaseCounterOfActiveWindows(unsigned delta) noexcept {
  active_windows_count_ += delta;
}

void gpu_renderer::Window::DecreaseCounterOfActiveWindows(unsigned delta) noexcept {
  active_windows_count_ -= delta;
}

unsigned gpu_renderer::Window::GetCountOfActiveWindows() noexcept {
  return active_windows_count_;
}