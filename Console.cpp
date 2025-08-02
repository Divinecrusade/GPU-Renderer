#include "Console.hpp"

#include <fcntl.h>
#include <io.h>

#include <cstdio>
#include <iostream>
#include <optional>
#include <string>

#include "DebugHeader.hpp"
#include "WinError.hpp"
#include "OptimisedWindowsHeader.hpp"

void gpu_renderer::Console::InitStdStreams(
    std::wstring_view console_window_title) {
  static Console instance{console_window_title};
#ifdef _DEBUG
  static bool std_streams_initialized{false};
  if (std_streams_initialized) {
    std::wcerr
        << L"Called InitStdStreans but "
        << L"Std streams (wclog, wcout, wcerr, wcin) are already initialized";
  }
#endif  // _DEBUG
  if (!instance.valid_) {
#ifdef _DEBUG
    throw exception::WinError{
        __FILEW__, __LINE__,
        "Error happened during instantiation of console window",
        GetLastError()};
#else
    throw exception::WinError{"Console was not created",
                                          GetLastError()};
#endif  // _DEBUG
  }
}

gpu_renderer::Console::Console(std::wstring_view console_window_title) noexcept {
  if (!AllocConsole()) {
    OutputDebugStringW(L"Console wasn't allocated\n");
    return;
  }
  FILE* cout_stream{nullptr};
  (void)freopen_s(&cout_stream, "CONOUT$", "w", stdout);
  FILE* cerr_stream{nullptr};
  (void)freopen_s(&cerr_stream, "CONOUT$", "w", stderr);
  FILE* cin_stream{nullptr};
  (void)freopen_s(&cin_stream, "CONIN$", "r", stdin);

  static constexpr UINT kCpUnicode{65001u};
  (void)SetConsoleOutputCP(kCpUnicode);
  (void)SetConsoleCP(kCpUnicode);

  (void)_setmode(_fileno(stdout), _O_U8TEXT);
  (void)_setmode(_fileno(stderr), _O_U8TEXT);

  (void)std::ios::sync_with_stdio(true);
  std::wcin.clear();
  std::wcout.clear();
  std::wcerr.clear();
  std::wclog.clear();

  valid_ = true;

  if (!SetConsoleTitleW(console_window_title.data())) {
    std::wcerr << L"Console title was not set, error code: " << GetLastError() << "\n";
  }
}

gpu_renderer::Console::~Console() {
  if (!FreeConsole()) {
    auto const err{GetLastError()};
    OutputDebugStringW(L"Console wasn't freed. Error code: ");
    OutputDebugStringW(std::to_wstring(err).c_str());
    OutputDebugStringW(L"\n");
  }
}
