#include "DebugHeader.hpp"
#include "OptimisedWindowsHeader.hpp"
#include "CachedDC.hpp"

#include <string>
#include <utility>
#include <cassert>

#ifdef DCONSOLE
#include <fcntl.h>
#include <io.h>
#include <cstdio>
#include <iostream>

#define DLOG_WIN_MSG
#endif  // DCONSOLE

#ifdef DLOG_WIN_MSG
#include "WinMsgFormatter.hpp"
#endif // DLOG_WIN_MSG

static LRESULT WINAPI WindowProcW(_In_ HWND hWnd, _In_ UINT Msg,
                                  _In_ WPARAM wParam, _In_ LPARAM lParam) {
#if defined(DCONSOLE) && defined(DLOG_WIN_MSG)
  static WinMsgFormatter msg_formatter{};
  std::wclog << msg_formatter(Msg, wParam, lParam) << std::endl;
#endif  // DCONSOLE && DLOG_WIN_MSG
  switch (Msg) {
    case WM_CLOSE: {
      PostQuitMessage(EXIT_SUCCESS);
    } break;
  }
  return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    [[maybe_unused]] _In_opt_ HINSTANCE,
                    [[maybe_unused]] _In_ LPWSTR, 
                    _In_ int nCmdShow) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF |
                 _CRTDBG_CHECK_ALWAYS_DF);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif // _DEBUG

#ifdef DCONSOLE
  if (BOOL const console_allocated{AllocConsole()}; !console_allocated) {
    return static_cast<int>(GetLastError());
  }
  FILE* cout_stream{nullptr};
  (void)freopen_s(&cout_stream, "CONOUT$", "w", stdout);
  FILE* cerr_stream{nullptr};
  (void)freopen_s(&cerr_stream, "CONOUT$", "w", stderr);
  FILE* cin_stream{nullptr};
  (void)freopen_s(&cin_stream, "CONIN$", "r", stdin);

  static constexpr UINT CP_UNICODE{65001u};
  (void)SetConsoleOutputCP(CP_UNICODE);
  (void)SetConsoleCP(CP_UNICODE);

  (void)_setmode(_fileno(stdout), _O_U8TEXT);
  (void)_setmode(_fileno(stderr), _O_U8TEXT);

  (void)std::ios::sync_with_stdio(true);
  std::wcin.clear();
  std::wcout.clear();
  std::wcerr.clear();
  std::wclog.clear();

  SetConsoleTitleW(L"GPU-Renderer Debug Console");
#endif  // DCONSOLE

  using gpu_renderer::CachedDC;

  CachedDC const wc{hInstance, WindowProcW};

  static constexpr DWORD NO_EXTRA_STYLE{NULL};
  static constexpr HWND NO_PARENT{NULL};
  static constexpr HMENU NO_MENU{NULL};
  static constexpr LPVOID NO_EXTRA_DATA{NULL};

  static constexpr auto WINDOW_EXTRA_STYLE{NO_EXTRA_STYLE};
  static constexpr LPCWSTR WINDOW_NAME{L"GPU-Renderer"};
  static constexpr DWORD WINDOW_STYLE{WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU};
  static constexpr int WINDOW_X_POS{30};
  static constexpr int WINDOW_Y_POS{60};
  static constexpr int WINDOW_WIDTH{640};
  static constexpr int WINDOW_HEIGHT{480};
  static constexpr auto WindowCreationFailed = [](HWND const& wnd) {
    return wnd == NULL;
  };
  HWND const hWnd{CreateWindowExW(WINDOW_EXTRA_STYLE, wc.CLASS_NAME, WINDOW_NAME,
                                  WINDOW_STYLE, WINDOW_X_POS, WINDOW_Y_POS,
                                  WINDOW_WIDTH, WINDOW_HEIGHT, NO_PARENT,
                                  NO_MENU, hInstance, NO_EXTRA_DATA)};
  if (WindowCreationFailed(hWnd)) {
    return static_cast<int>(GetLastError());
  }
  (void)ShowWindow(hWnd, nCmdShow);

  static constexpr HWND ALL_WINDOWS{NULL};
  static constexpr UINT NO_MIN_RANGE_FILTER_MSG{NULL};
  static constexpr UINT NO_MAX_RANGE_FILTER_MSG{NULL};
  MSG msg{};
  static constexpr auto ErrorHappened = [](BOOL result) {
    return result == -1;
  };
  while (auto operation_done{GetMessageW(
      &msg, ALL_WINDOWS, NO_MIN_RANGE_FILTER_MSG, NO_MAX_RANGE_FILTER_MSG)}) {
    if (ErrorHappened(operation_done)) {
      return static_cast<int>(GetLastError());
    } else {
      (void)TranslateMessage(&msg);
      (void)DispatchMessageW(&msg);
    }
  }

#ifdef DCONSOLE
  if (!FreeConsole()) {
    auto const err{GetLastError()};
    OutputDebugStringW(std::to_wstring(err).c_str());
    OutputDebugStringW(L"\n");
  }
#endif  // DCONSOLE

  assert(_CrtDumpMemoryLeaks() == FALSE);
  return static_cast<int>(msg.wParam);
}
