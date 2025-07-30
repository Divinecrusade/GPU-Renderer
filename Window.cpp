#include "Window.hpp"

#include "DebugHeader.hpp"

#ifdef DCONSOLE
#include <iostream>

#include "WinMsgFormatter.hpp"
#endif  // DCONSOLE
#include <cassert>

bool gpu_renderer::Window::first_show_done_{false};

gpu_renderer::Window::Window(WindowClass const& window_class,
                             LPCWSTR lpszWindowName, DWORD dwStyle, int x,
                             int y, int nWidth, int nHeight,
                             HINSTANCE hInstance, DWORD dwExStyle) noexcept {
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
  AdjustWindowRectEx(&window_pos, dwStyle, FALSE, dwExStyle);

  hwnd_ = CreateWindowExW(dwExStyle, window_class.GetLpClassName(),
                          lpszWindowName, dwStyle, window_pos.left,
                          window_pos.top, window_pos.right - window_pos.left,
                          window_pos.bottom - window_pos.top, kNoParent,
                          kNoMenu, hInstance, reinterpret_cast<LPVOID>(this));

  if (WindowCreationFailed(hwnd_)) {
#ifdef DCONSOLE
    std::wcerr << L"Window creation failed\n";
    std::wcerr << L"Error code: " << GetLastError() << L"\n";
#endif  // DCONSOLE
    return;
  }

#ifdef DCONSOLE
  std::wclog << L"Window created successfully with handle\n";
#endif  // DCONSOLE
}

gpu_renderer::Window::~Window() noexcept {
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
  if (!DestroyWindow(hwnd_)) {
#ifdef DCONSOLE
    std::wcerr << L"Window destruction failed\n";
    std::wcerr << L"Error code: " << GetLastError() << L"\n";
#endif  // DCONSOLE
  } else {
#ifdef DCONSOLE
    std::wclog << L"Window destroyed successfully\n";
#endif  // DCONSOLE
  }
}

void gpu_renderer::Window::Show() noexcept {
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
  assert(((void)"Window must be showed first with wWinaMain nCmdShow param",
          first_show_done_));
  (void)ShowWindow(hwnd_, SW_SHOW);
}

void gpu_renderer::Window::Show(int nCmdShow) noexcept {
  assert(((void)"HWND cannot be null", hwnd_ != NULL));
  if (first_show_done_) {
#ifdef DCONSOLE
    std::wcerr << L"Warning: This overloaded option of Show called but "
                  L"application first show already occurred\n";
#endif  // DCONSOLE
    return Show();
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
#ifdef DCONSOLE
    std::wclog << L"Installing wndproc for new instance of Window...\n";
    std::wclog << WinMsgFormatter{}(Msg, wParam, lParam) << "\n";
#endif  // DCONSOLE
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
#ifdef DCONSOLE
    std::wclog << L"Done installing wndproc for new instance of Window\n";
#endif  // DCONSOLE
  }
  return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

LRESULT WINAPI gpu_renderer::Window::DisptachWindowProcW(
    _In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam,
    _In_ LPARAM lParam) noexcept {
#ifdef DCONSOLE
  std::wclog << L"Dispatcher get windows message...\n";
  std::wclog << WinMsgFormatter{}(Msg, wParam, lParam) << "\n";
#endif  // DCONSOLE
  Window* const window_instance{
      reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA))};
  assert(((void)"Window data must have the pointer to Window class object",
          window_instance != nullptr));
#ifdef DCONSOLE
  std::wclog << L"...forward this message to Window object\n";
#endif  // DCONSOLE
  return window_instance->HandleWinMessage(Msg, wParam, lParam);
}

LRESULT gpu_renderer::Window::HandleWinMessage(UINT Msg, WPARAM wParam,
                                               LPARAM lParam) {
#ifdef DCONSOLE
  std::wclog << L"Message catched in Window object\n";
#endif  // DCONSOLE
  switch (Msg) {
    case WM_CLOSE: {
      PostQuitMessage(EXIT_SUCCESS);
      return 0;
    }
  }
  return DefWindowProcW(hwnd_, Msg, wParam, lParam);
}