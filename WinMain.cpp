#ifdef _DEBUG
#define _ALLOW_RTCc_IN_STL
#include <cassert>
#define __CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define DEBUG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif  // _DEBUG

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define STRICT

#define NOGDICAPMASKS
#define NOMENUS
#define NOICONS
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NONLS
#define NOMETAFILE
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE
#pragma warning(push)
#pragma warning(disable : 4005 4668 5039 4514 4820 4711)
#pragma warning(disable : 6001 6011 6387)
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#pragma warning(pop)

#include <iostream>
#include <cstdio>
#include <utility>

#ifdef _DEBUG
#define DCONSOLE
#endif  // _DEBUG

static LRESULT WINAPI WindowProcW(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
  switch (Msg) {
    case WM_CLOSE:
    {
      PostQuitMessage(EXIT_SUCCESS);
    }
    break;
  }
  return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    [[maybe_unused]] _In_opt_ HINSTANCE,
                    [[maybe_unused]] _In_ LPWSTR,
                    _In_ int nCmdShow) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | 
                 _CRTDBG_LEAK_CHECK_DF |
                 _CRTDBG_CHECK_ALWAYS_DF);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif

#ifdef DCONSOLE
  if (BOOL const console_allocated{AllocConsole()}; !console_allocated) {
    return static_cast<int>(GetLastError());
  }
  FILE* cout_stream{nullptr};
  (void) freopen_s(&cout_stream, "CONOUT$", "w", stdout);
  FILE* cerr_stream{nullptr};
  (void) freopen_s(&cerr_stream, "CONOUT$", "w", stderr);
  FILE* cin_stream{nullptr};
  (void) freopen_s(&cin_stream, "CONIN$", "r", stdin);
  
  std::ios::sync_with_stdio(true);
  std::cin.clear();
  std::cout.clear();
  std::cerr.clear();
  std::clog.clear();

  SetConsoleTitleW(L"GPU-Renderer Debug Console");
#endif  // DCONSOLE

  static constexpr HICON DEFAULT_ICON{NULL};
  static constexpr HCURSOR DEFAULT_CURSOR{NULL};
  static constexpr HBRUSH NO_BACKGROUND{NULL};
  static constexpr LPCWSTR NO_MENU_NAME{NULL};
  static constexpr LPCWSTR CLASS_NAME{L"Canvas for DirectX"};
  WNDCLASSEXW const wc{.cbSize = sizeof(WNDCLASSEX),
                       .style = CS_OWNDC,
                       .lpfnWndProc = WindowProcW,
                       .cbClsExtra = 0,
                       .cbWndExtra = 0,
                       .hInstance = hInstance,
                       .hIcon = DEFAULT_ICON,
                       .hCursor = DEFAULT_CURSOR,
                       .hbrBackground = NO_BACKGROUND,
                       .lpszMenuName = NO_MENU_NAME,
                       .lpszClassName = CLASS_NAME,
                       .hIconSm = DEFAULT_ICON};

  static constexpr auto ClassRegistrationFailed = [](ATOM aClass) {
    return aClass == NULL;
  };
  ATOM const aClass{RegisterClassExW(&wc)};
  if (ClassRegistrationFailed(aClass)) {
    return static_cast<int>(GetLastError());
  }

  static constexpr DWORD NO_EXTRA_STYLE{NULL};
  static constexpr HWND NO_PARENT{NULL};
  static constexpr HMENU NO_MENU{NULL};
  static constexpr LPVOID NO_EXTRA_DATA{NULL};

  static constexpr auto WINDOW_EXTRA_STYLE{NO_EXTRA_STYLE};
  //auto const WINDOW_CLASS{MAKEINTATOM(aClass)};
  static constexpr LPCWSTR WINDOW_CLASS{CLASS_NAME};
  static constexpr LPCWSTR WINDOW_NAME{L"GPU-Renderer"};
  static constexpr DWORD WINDOW_STYLE{WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU};
  static constexpr int WINDOW_X_POS{30};
  static constexpr int WINDOW_Y_POS{60};
  static constexpr int WINDOW_WIDTH{640};
  static constexpr int WINDOW_HEIGHT{480};
  static constexpr auto WindowCreationFailed = [](HWND const& wnd) {
    return wnd == NULL;
  };
  HWND const hWnd{CreateWindowExW(WINDOW_EXTRA_STYLE, WINDOW_CLASS, WINDOW_NAME,
                                  WINDOW_STYLE, WINDOW_X_POS, WINDOW_Y_POS,
                                  WINDOW_WIDTH, WINDOW_HEIGHT, NO_PARENT,
                                  NO_MENU, hInstance, NO_EXTRA_DATA)};
  if (WindowCreationFailed(hWnd)) {
    return static_cast<int>(GetLastError());
  }
  (void) ShowWindow(hWnd, nCmdShow);

  static constexpr HWND ALL_WINDOWS{NULL};
  static constexpr UINT NO_MIN_RANGE_FILTER_MSG{NULL};
  static constexpr UINT NO_MAX_RANGE_FILTER_MSG{NULL};
  MSG msg{};
  static constexpr auto ErrorHappened = [](BOOL result) {
    return result == -1;
  };
  while (auto operation_done{GetMessageW(&msg, ALL_WINDOWS, NO_MIN_RANGE_FILTER_MSG, NO_MAX_RANGE_FILTER_MSG)}) {
    if (ErrorHappened(operation_done)) {
      return static_cast<int>(GetLastError());
    } else {
      (void) TranslateMessage(&msg);
      (void) DispatchMessageW(&msg);
    }
  }

#ifdef DCONSOLE
  (void) FreeConsole();
#endif  // DCONSOLE

  assert(_CrtDumpMemoryLeaks() == FALSE);
  return static_cast<int>(msg.wParam);
}
