#include "Window.hpp"
#include "WinError.hpp"
#include "DebugHeader.hpp"

#if defined(LOG_WINDOW_MESSAGES) || defined(LOG_WINDOW)
#include <iostream>
#endif  // LOG_WINDOW_MESSAGES || LOG_WINDOW
#ifdef LOG_WINDOW_MESSAGES
#include "WinMsgFormatter.hpp"
#endif  // LOG_WINDOW_MESSAGES
#include <cassert>

bool gpu_renderer::Window::first_show_done_{false};

gpu_renderer::Window::Window(WindowClass const& window_class,
                             LPCWSTR lpszWindowName, DWORD dwStyle, int x,
                             int y, int nWidth, int nHeight,
                             HINSTANCE hInstance, DWORD dwExStyle) {
  assert(((void)"Window name cannot be nullptr", lpszWindowName != nullptr));
  assert(((void)"Instance handle cannot be null", hInstance != NULL));
  assert(((void)"Width must be positive", nWidth > 0));
  assert(((void)"Height must be positive", nHeight > 0));

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
  if (!DestroyWindow(hwnd_)) {
#ifdef LOG_WINDOW
    std::wcerr << L"Window destruction failed\n";
    std::wcerr << L"Error code: " << GetLastError() << L"\n";
#endif  // LOG_WINDOW
  }
#ifdef LOG_WINDOW
  else {
    std::wclog << L"Window destroyed successfully\n";
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
  if (first_show_done_) {
#ifdef LOG_WINDOW
    std::wcerr << L"Warning: This overloaded option of Show called but "
                  L"application first show already occurred\n";
#endif  // LOG_WINDOW
  }

  (void)ShowWindow(hwnd_, nCmdShow);
  first_show_done_ = true;
}

void gpu_renderer::Window::Hide() const noexcept {
  assert(((void)"Window must be showed first with wWinaMain nCmdShow param",
          first_show_done_));
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
  (void)ShowWindow(hwnd_, SW_HIDE);
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
#ifdef LOG_WINDOW
    std::wclog << L"Done installing wndproc for new instance of Window\n";
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
  return window_instance->HandleWinMessage(Msg, wParam, lParam);
}

LRESULT gpu_renderer::Window::HandleWinMessage(UINT Msg, WPARAM wParam,
                                               LPARAM lParam) noexcept {
#ifdef LOG_WINDOW_MESSAGES
  std::wclog << L"Message catched in Window object\n";
#endif  // LOG_WINDOW_MESSAGES
  switch (Msg) {
    case WM_CLOSE: {
      PostQuitMessage(EXIT_SUCCESS);
      return 0;
    }
  }
  return DefWindowProcW(hwnd_, Msg, wParam, lParam);
}