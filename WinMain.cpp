#include "DebugHeader.hpp"
#include "OptimisedWindowsHeader.hpp"
#include "CachedDC.hpp"
#include "Canvas.hpp"
#ifdef DCONSOLE
#include "Console.hpp"
#endif // DCSONSOLE

#include <string>
#include <utility>
#include <cassert>

#ifdef DCONSOLE
#define DLOG_WIN_MSG
#endif  // DCONSOLE

#ifdef DLOG_WIN_MSG
#include "WinMsgFormatter.hpp"
#endif // DLOG_WIN_MSG

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
  gpu_renderer::Console::InitStdStreams(L"GPU-Renderer Debug Console");
#endif

  using gpu_renderer::CachedDC;
  using gpu_renderer::Canvas;
  CachedDC wc{hInstance, Canvas::GetlpfnWndProc()};

  static constexpr LPCWSTR kWindowName{L"GPU-Renderer"};
  static constexpr POINT kLeftTopCornerPos{15L, 30L};
  static constexpr LONG kWindowWidth{640L};
  static constexpr LONG kWindowHeight{480L};
  Canvas wnd{wc, kWindowName, kLeftTopCornerPos, {kWindowWidth, kWindowHeight}, hInstance};
  wnd.Show(nCmdShow);

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

  assert(_CrtDumpMemoryLeaks() == FALSE);
  return static_cast<int>(msg.wParam);
}
