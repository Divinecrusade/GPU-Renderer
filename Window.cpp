#include "Window.hpp"

#include <cassert>

#include "WinError.hpp"
#if defined(LOG_WINDOW_MESSAGES) || defined(LOG_WINDOW)
#include <iostream>
#endif  // LOG_WINDOW_MESSAGES || LOG_WINDOW

#include <gsl/gsl>
#ifdef LOG_WINDOW_MESSAGES
#include "WinMsgFormatter.hpp"
#endif  // LOG_WINDOW_MESSAGES

namespace gpu_renderer::window {
unsigned Window::active_windows_count_{0u};
#ifdef _DEBUG
bool Window::first_show_done_{false};
#endif  // _DEBUG

Window::Window(WindowClass const& window_class,
                             LPCWSTR lpszWindowName, DWORD dwStyle, int x,
                             int y, int nWidth, int nHeight,
                             HINSTANCE hInstance, DWORD dwExStyle)
    : width_{nWidth}, height_{nHeight} {
  hwnd_ =
      InitializeWindow(this, window_class.GetLpClassName(), lpszWindowName,
                       dwStyle, x, y, width_, height_, hInstance, dwExStyle);
  assert(((void)"Window handler cannot be NULL", hwnd_ != NULL));
}

Window::~Window() noexcept {
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
  if (DestroyWindow(hwnd_)) {
#ifdef LOG_WINDOW
    try {
      std::wclog << L"Window destroyed successfully\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log Window destructor\n");
    }
#endif  // LOG_WINDOW
  }
#ifdef LOG_WINDOW
  else {
    try {
      std::wcerr << L"Window destruction failed. " << L"Error code: "
                 << GetLastError() << L"\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log Window destructor\n");
    }
  }
#endif  // LOG_WINDOW
}

void Window::Show() const noexcept {
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
  assert(((void)"Window must be showed first with wWinaMain nCmdShow param",
          first_show_done_));
  std::ignore = ShowWindow(hwnd_, SW_SHOW);
}

void Window::Show(int nCmdShow) const noexcept {
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
#ifdef _DEBUG
  if (first_show_done_) {
#ifdef LOG_WINDOW
    try {
      std::wcerr << L"Warning: This overloaded option of Show called but "
                    L"application first show already occurred\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log Window Show\n");
    }
#endif  // LOG_WINDOW
  }
#endif  // _DEBUG

  std::ignore = ShowWindow(hwnd_, nCmdShow);
#ifdef _DEBUG
  first_show_done_ = true;
#endif  // _DEBUG
}

void Window::Hide() const noexcept {
  assert(((void)"Window must be showed first with wWinaMain nCmdShow param",
          first_show_done_));
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
  std::ignore = ShowWindow(hwnd_, SW_HIDE);
}

void Window::Enable() const noexcept {
  if (BOOL const was_disabled{EnableWindow(hwnd_, TRUE)}; !was_disabled) {
#ifdef LOG_WINDOW
    try {
      std::wcerr
          << L"Enable method called, but window has been enabled already\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log Window Enable\n");
    }
#endif  // LOG_WINDOW
  }
}

void Window::Disable() const noexcept {
  if (BOOL const was_disabled{EnableWindow(hwnd_, FALSE)}; was_disabled) {
#ifdef LOG_WINDOW
    try {
      std::wcerr
          << L"Disable method called, but window has been disabled already\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log Window Disable\n");
    }
#endif  // LOG_WINDOW
  }
}

bool Window::IsEnabled() const noexcept {
  return IsWindowEnabled(hwnd_);
}

bool Window::IsShown() const noexcept {
  return IsWindowVisible(hwnd_);
}

Keyboard::View Window::GetKeyboard() noexcept {
  return kbd_;
}

Mouse::View Window::GetMouse() noexcept { 
  return mse_; 
}

WNDPROC Window::GetlpfnWndProc() noexcept {
  return SetupWindowProcW; 
}


int Window::LockInMessageQueue() {  
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
      std::ignore = TranslateMessage(&msg); // TODO: check its return
      std::ignore = DispatchMessageW(&msg); // TODO: check its return
    }
  }

  return gsl::narrow<ExitCode>(msg.wParam);
}

std::optional<ExitCode> Window::ProcessMessagesFromQueue() {
  static MSG msg{};

  while (PeekMessageW(&msg, kAllWindows, kNoMinRangeFilterMsg,
                      kNoMaxRangeFilterMsg, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      return gsl::narrow<ExitCode>(msg.wParam);
    }
    std::ignore = DispatchMessageW(&msg); // TODO: check its return
  }
  return std::nullopt;
}

HWND Window::InitializeWindow(Window* window_instance, 
                                            LPCWSTR lpClassName,
                                            LPCWSTR lpszWindowName,
                                            DWORD dwStyle, int x, int y,
                                            int nWidth, int nHeight,
                                            HINSTANCE hInstance,
                                            DWORD dwExStyle) {
  assert(((void)"Window name cannot be nullptr", lpszWindowName != nullptr));
  assert(((void)"Instance handle cannot be null", hInstance != NULL));
  assert(((void)"Width must be positive", nWidth > 0));
  assert(((void)"Height must be positive", nHeight > 0));
  assert(((void)"Current implementation of window doesn't support child window "
                "type",
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
    try {
      std::wcerr << L"AdjustWindowRectEx failed during Window constructing,"
                 << L"error code: " << GetLastError() << "\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log Window constructor\n");
    }
#endif  // LOG_WINDOW
  }
  HWND const hwnd{CreateWindowExW(
      dwExStyle, lpClassName, lpszWindowName, dwStyle,
      window_pos.left, window_pos.top, window_pos.right - window_pos.left,
      window_pos.bottom - window_pos.top, kNoParent, kNoMenu, hInstance, window_instance)};

  if (WindowCreationFailed(hwnd)) {
#ifdef LOG_WINDOW
    try {
      std::wcerr << L"Window creation failed\n";
      std::wcerr << L"Error code: " << GetLastError() << L"\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log Window constructor\n");
    }
#endif  // LOG_WINDOW
#ifdef _DEBUG
    throw exception::WinError{__FILEW__, __LINE__,
                              "CreateWindowExW returned NULL hwnd",
                              GetLastError()};
#else
    throw exception::WinError{"It was not able to create window",
                              GetLastError()};
#endif  // _DEBUG
  }

#ifdef LOG_WINDOW
  try {
    std::wclog << L"Window '" << lpszWindowName << L"' of class '"
               << lpClassName << L"' created successfully\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Window constructor\n");
  }
#endif  // LOG_WINDOW
  return hwnd;
}

LRESULT WINAPI Window::SetupWindowProcW(
    _In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam,
    _In_ LPARAM lParam) noexcept {
  if (Msg == WM_CREATE) {
#ifdef LOG_WINDOW_MESSAGES
    try {
      std::wclog << L"Installing wndproc for new instance of Window...\n";
      std::wclog << debug::WinMsgFormatter{}(Msg, wParam, lParam) << "\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log Window SetupWindowProcW\n");
    }
#endif  // LOG_WINDOW_MESSAGES
#pragma warning(push)
#pragma warning(disable : 26490)
    CREATESTRUCTW const* const creation_params{
        reinterpret_cast<CREATESTRUCTW const*>(lParam)};
    assert(((void)"lParam must be a pointer to window creation struct",
            creation_params != nullptr));
    Window* const window_instance{
        static_cast<Window*>(creation_params->lpCreateParams)};
    assert(((void)"Window creation struct must have the pointer to Window "
                  "class object",
            window_instance != nullptr));
    std::ignore = SetWindowLongPtrW(
        hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window_instance));
    std::ignore = SetWindowLongPtrW(
        hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(DisptachWindowProcW));
#pragma warning(pop)
    ++active_windows_count_;
#ifdef LOG_WINDOW
    try {
      std::wclog << L"Done installing wndproc for new instance of Window\n";
      std::wclog << L"There're " << active_windows_count_
                 << L" active windows right now\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log Window SetupWindowProcW\n");
    }
#endif  // LOG_WINDOW
  }
  return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

LRESULT WINAPI Window::DisptachWindowProcW(
    _In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam,
    _In_ LPARAM lParam) noexcept {
#ifdef LOG_WINDOW_MESSAGES
  try {
    std::wclog << L"Dispatcher get windows message...\n";
    std::wclog << debug::WinMsgFormatter{}(Msg, wParam, lParam) << "\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Window DisptachWindowProcW\n");
  }
#endif  // LOG_WINDOW_MESSAGES
#pragma warning(push)
#pragma warning(disable : 26490)
  Window* const window_instance{
      reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA))};
#pragma warning(pop)
  assert(((void)"Window data must have the pointer to Window class object",
          window_instance != nullptr));
#ifdef LOG_WINDOW_MESSAGES
  try {
    std::wclog << L"...forward this message to Window object\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Window DisptachWindowProcW\n");
  }
#endif  // LOG_WINDOW_MESSAGES
  return window_instance->HandleMessage(Msg, wParam, lParam);
}

Window::Window(std::size_t keyboard_events_queue_size,
                             std::size_t keyboard_chars_buffer_size,
                             std::size_t mouse_events_queue_size,
                             WindowClass const& window_class,
                             LPCWSTR lpszWindowName, DWORD dwStyle, int x,
                             int y, int nWidth, int nHeight,
                             HINSTANCE hInstance, DWORD dwExStyle)
    : width_{nWidth},
      height_{nHeight},
      kbd_{keyboard_events_queue_size, keyboard_chars_buffer_size},
      mse_{mouse_events_queue_size} {
  hwnd_ =
      InitializeWindow(this, window_class.GetLpClassName(), lpszWindowName,
                       dwStyle, x, y, width_, height_, hInstance, dwExStyle);
  assert(((void)"Window handler cannot be NULL", hwnd_ != NULL));
}

LRESULT Window::HandleMessage(UINT Msg, WPARAM wParam,
                                            LPARAM lParam) noexcept {
  static constexpr auto kPreviousKeyStateMask{
      0b1000000000000000000000000000000};
#ifdef LOG_WINDOW_MESSAGES
  try {
    std::wclog << L"Message catched in Window object\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
  }
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
      try {
        std::wclog << L"Window disabled and hidden (inactive). There're "
                   << active_windows_count_ << L" active windows right now\n";
      } catch (...) {
        OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
      }
#endif  // LOG_WINDOW
      if (active_windows_count_ == 0u) {
#ifdef LOG_WINDOW
        try {
          std::wclog << L"No active windows, posting quit message queue...\n";
        } catch (...) {
          OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
        }
#endif  // LOG_WINDOW
        PostQuitMessage(EXIT_SUCCESS);
      }
      return 0;
    }
    case WM_KILLFOCUS: {
      kbd_.ClearKeysState();
      try {
        kbd_.ClearEventsQueue();
      } catch ([[maybe_unused]] std::exception const& e) {
#ifdef LOG_WINDOW
        try {
          std::string const narrow_what{e.what()};
          std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};
          std::wcerr << L"Exception raised during Keyboard "
                        L"ClearEventsQueue. What happened:"
                     << wide_what << "\n";
        } catch (...) {
          OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
        }
#endif  // LOG_WINDOW
        OutputDebugStringW(L"Keyboard ClearEventsQueue raised exception\n");
      }
    } break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
      if (!(lParam & kPreviousKeyStateMask) || kbd_.IsAutoRepeating()) {
        try {
          kbd_.OnKeyDown(gsl::narrow_cast<unsigned char>(wParam));
        } catch ([[maybe_unused]] std::exception const& e) {
#ifdef LOG_WINDOW
          try {
            std::string const narrow_what{e.what()};
            std::wstring const wide_what{narrow_what.begin(),
                                         narrow_what.end()};
            std::wcerr << L"Exception raised during Keyboard "
                          L"OnKeyDown. What happened:"
                       << wide_what << "\n";
          } catch (...) {
            OutputDebugStringW(
                L"Exception raised in log Window HandleMessage\n");
          }
#endif  // LOG_WINDOW
          OutputDebugStringW(L"Keyboard OnKeyDown raised exception\n");
        }
      }
    } break;
    case WM_KEYUP: {
      try {
        kbd_.OnKeyUp(gsl::narrow_cast<unsigned char>(wParam));
      } catch ([[maybe_unused]] std::exception const& e) {
#ifdef LOG_WINDOW
        try {
          std::string const narrow_what{e.what()};
          std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};
          std::wcerr << L"Exception raised during Keyboard "
                        L"OnKeyUp. What happened:"
                     << wide_what << "\n";
        } catch (...) {
          OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
        }
#endif  // LOG_WINDOW
        OutputDebugStringW(L"Keyboard OnKeyUp raised exception\n");
      }
    } break;
    case WM_CHAR: {
      try {
        kbd_.OnChar(gsl::narrow_cast<wchar_t>(wParam));
      } catch ([[maybe_unused]] std::exception const& e) {
#ifdef LOG_WINDOW
        try {
          std::string const narrow_what{e.what()};
          std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};
          std::wcerr << L"Exception raised during Keyboard "
                        L"OnChar. What happened:"
                     << wide_what << "\n";
        } catch (...) {
          OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
        }
#endif  // LOG_WINDOW
        OutputDebugStringW(L"Keyboard OnChar raised exception\n");
      }
    } break;
    case WM_LBUTTONDOWN: {
      try {
        mse_.OnLButtonDown(lParam);
      } catch ([[maybe_unused]] std::exception const& e) {
#ifdef LOG_WINDOW
        try {
          std::string const narrow_what{e.what()};
          std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};
          std::wcerr << L"Exception raised during Mouse "
                        L"OnLButtonDown. What happened:"
                     << wide_what << "\n";
        } catch (...) {
          OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
        }
#endif  // LOG_WINDOW
        OutputDebugStringW(L"Mouse OnLButtonDown raised exception\n");
      }
    } break;
    case WM_LBUTTONUP: {
      try {
        mse_.OnLButtonUp(lParam);
      } catch ([[maybe_unused]] std::exception const& e) {
#ifdef LOG_WINDOW
        try {
          std::string const narrow_what{e.what()};
          std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};
          std::wcerr << L"Exception raised during Mouse "
                        L"OnLButtonUp. What happened:"
                     << wide_what << "\n";
        } catch (...) {
          OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
        }
#endif  // LOG_WINDOW
        OutputDebugStringW(L"Mouse OnLButtonUp raised exception\n");
      }
    } break;
    case WM_RBUTTONDOWN: {
      try {
        mse_.OnRButtonDown(lParam);
      } catch ([[maybe_unused]] std::exception const& e) {
#ifdef LOG_WINDOW
        try {
          std::string const narrow_what{e.what()};
          std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};
          std::wcerr << L"Exception raised during Mouse "
                        L"OnRButtonDown. What happened:"
                     << wide_what << "\n";
        } catch (...) {
          OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
        }
#endif  // LOG_WINDOW
        OutputDebugStringW(L"Mouse OnRButtonDown raised exception\n");
      }
    } break;
    case WM_RBUTTONUP: {
      try {
        mse_.OnRButtonUp(lParam);
      } catch ([[maybe_unused]] std::exception const& e) {
#ifdef LOG_WINDOW
        try {
          std::string const narrow_what{e.what()};
          std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};
          std::wcerr << L"Exception raised during Mouse "
                        L"OnRButtonUp. What happened:"
                     << wide_what << "\n";
        } catch (...) {
          OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
        }
#endif  // LOG_WINDOW
        OutputDebugStringW(L"Mouse OnRButtonUp raised exception\n");
      }
    } break;
    case WM_MOUSEMOVE: {
      auto const mse_cur_pos{MAKEPOINTS(lParam)};
      try {
        if (mse_cur_pos.x >= 0 && mse_cur_pos.x < width_ &&
            mse_cur_pos.y >= 0 && mse_cur_pos.y < height_) {
          mse_.OnMove(lParam);
          if (Mouse::View const v{mse_}; !v.IsInWindow()) {
            SetCapture(hwnd_);
            mse_.OnHoverWindow(lParam);
          }
        } else {
          if (Mouse::View const v{mse_};
              v.IsLeftButtonPressed() || v.IsRightButtonPressed()) {
            mse_.OnMove(lParam);
          } else if (v.IsInWindow()) {
            ReleaseCapture();
            mse_.OnLeaveWindow(lParam);
          }
        }
      } catch ([[maybe_unused]] std::exception const& e) {
#ifdef LOG_WINDOW
        try {
          std::string const narrow_what{e.what()};
          std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};
          std::wcerr << L"Exception raised during mouse move handling. "
                        L"What happened:"
                     << wide_what << "\n";
        } catch (...) {
          OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
        }
#endif  // LOG_WINDOW
        OutputDebugStringW(L"Handling mouse move raised exception\n");
      }
    } break;
    case WM_MOUSEWHEEL: {
      try {
        mse_.OnWheel(lParam, wParam);
      } catch ([[maybe_unused]] std::exception const& e) {
#ifdef LOG_WINDOW
        try {
          std::string const narrow_what{e.what()};
          std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};
          std::wcerr << L"Exception raised during Mouse "
                        L"OnWheel. What happened:"
                     << wide_what << "\n";
        } catch (...) {
          OutputDebugStringW(L"Exception raised in log Window HandleMessage\n");
        }
#endif  // LOG_WINDOW
        OutputDebugStringW(L"Mouse OnWheel raised exception\n");
      }
    } break;
    default:
      return DefWindowProcW(hwnd_, Msg, wParam, lParam);
  }
  return DefWindowProcW(hwnd_, Msg, wParam, lParam);
}

HWND Window::GetHWND() const noexcept { 
  return hwnd_; 
}

void Window::IncreaseCounterOfActiveWindows(
    unsigned delta) noexcept {
  active_windows_count_ += delta;
}

void Window::DecreaseCounterOfActiveWindows(
    unsigned delta) noexcept {
  active_windows_count_ -= delta;
}

unsigned Window::GetCountOfActiveWindows() noexcept {
  return active_windows_count_;
}
}  // namespace gpu_renderer::window
