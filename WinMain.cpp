#include "CachedDC.hpp"
#include "Canvas.hpp"
#ifdef _DEBUG
#include "Console.hpp"
#endif  // DCSONSOLE
#include "OptimisedWindowsHeader.hpp"
#include "WinError.hpp"

#include <gsl/gsl>

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    [[maybe_unused]] _In_opt_ HINSTANCE,
                    [[maybe_unused]] _In_ LPWSTR, _In_ int nCmdShow) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF |
                 _CRTDBG_CHECK_ALWAYS_DF);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif  // _DEBUG

  MSG msg{};
  using gpu_renderer::window::CachedDC;
  using gpu_renderer::window::Canvas;
  using gpu_renderer::exception::WinError;
  using gpu_renderer::debug::Console;
  try {
#ifdef _DEBUG
    Console::InitStdStreams(L"GPU-Renderer Debug Console");
#endif  // _DEBUG

    CachedDC wc{hInstance, Canvas::GetlpfnWndProc()};
    static constexpr LPCWSTR kWindowName{L"GPU-Renderer"};
    static constexpr std::pair kLeftTopCornerPos{15, 30};
    static constexpr int kWindowWidth{640L};
    static constexpr int kWindowHeight{480L};
    Canvas wnd{wc,
               kWindowName,
               kLeftTopCornerPos.first,
               kLeftTopCornerPos.second,
               kWindowWidth,
               kWindowHeight,
               hInstance};
    wnd.Show(nCmdShow);

    static constexpr HWND ALL_WINDOWS{NULL};
    static constexpr UINT NO_MIN_RANGE_FILTER_MSG{NULL};
    static constexpr UINT NO_MAX_RANGE_FILTER_MSG{NULL};
    static constexpr auto ErrorHappened = [](BOOL result) {
      return result == -1;
    };
    while (auto const operation_done{GetMessageW(
        &msg, ALL_WINDOWS, NO_MIN_RANGE_FILTER_MSG, NO_MAX_RANGE_FILTER_MSG)}) {
      if (ErrorHappened(operation_done)) {
#ifdef _DEBUG
        throw WinError{__FILEW__, __LINE__, "Handling message provokes error",
                       GetLastError()};
#else
        throw WinError{"Event goes wrong", GetLastError()};
#endif  // _DEBUG
      } else {
        (void)TranslateMessage(&msg);
        (void)DispatchMessageW(&msg);
      }
    }
  } catch (WinError const& e) {
    MessageBoxW(NULL, e.WhatHappened().c_str(), e.kTypeOfException.data(),
                MB_OK | MB_ICONERROR);
    return static_cast<int>(e.GetErrorCode());
  } catch (std::exception const& e) {
    std::string const narrow{e.what()};
    MessageBoxW(NULL, std::wstring{narrow.begin(), narrow.end()}.c_str(),
                L"C++ standard exception", MB_OK | MB_ICONERROR);
    return EXIT_FAILURE;
  } catch (...) {
    MessageBoxW(NULL, L"Something unexpected happened", L"Unknown error",
                MB_OK | MB_ICONERROR);
    return EXIT_FAILURE;
  }

  if constexpr (sizeof(WPARAM) > sizeof(int)) {
    return gsl::narrow<int>(msg.wParam);
  } else {
    return int(msg.wParam);
  }
}
