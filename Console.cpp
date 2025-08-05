#include "Console.hpp"

#include <fcntl.h>
#include <io.h>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>

#include "OptimisedWindowsHeader.hpp"
#include "WinError.hpp"

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
}

gpu_renderer::Console::Console(std::wstring_view console_window_title) {
  if (!AllocConsole()) {
#ifdef _DEBUG
    throw exception::WinError{__FILEW__, __LINE__, "Console wasn't allocated",
                              GetLastError()};
#else
    throw exception::WinError{"Console wasn't allocated", GetLastError()};
#endif  // _DEBUG
  }
  FILE* cout_stream{nullptr};
  std::ignore = freopen_s(&cout_stream, "CONOUT$", "w", stdout);
  FILE* cerr_stream{nullptr};
  std::ignore = freopen_s(&cerr_stream, "CONOUT$", "w", stderr);
  FILE* cin_stream{nullptr};
  std::ignore = freopen_s(&cin_stream, "CONIN$", "r", stdin);

  static constexpr UINT kCpUnicode{65001u};
  if (!SetConsoleOutputCP(kCpUnicode)) {
#ifdef _DEBUG
    throw exception::WinError{__FILEW__, __LINE__,
                              "Console output code page was not set to Unicode",
                              GetLastError()};
#else
    throw exception::WinError{"Console code page was not properly set",
                              GetLastError()};
#endif  // _DEBUG
  }
  if (!SetConsoleCP(kCpUnicode)) {
#ifdef _DEBUG
    throw exception::WinError{__FILEW__, __LINE__,
                              "Console input code page was not set to Unicode",
                              GetLastError()};
#else
    throw exception::WinError{"Console code page was not properly set",
                              GetLastError()};
#endif  // _DEBUG
  }

  static constexpr auto SetModeFailed = [](auto op_status) {
    return op_status < 0;
  };
  if (SetModeFailed(_setmode(_fileno(stdout), _O_U8TEXT))) {
    assert(((void)"Console out/log setmode failed",
            true));  // TODO: replace with corresponding exception
                     //       for errno
  }
  if (SetModeFailed(_setmode(_fileno(stderr), _O_U8TEXT))) {
    assert(((void)"Console err setmode failed",
            true));  // TODO: replace with corresponding exception
                     //       for errno
  }

  std::ignore = std::ios::sync_with_stdio(true);
  std::wcin.clear();
  std::wcout.clear();
  std::wcerr.clear();
  std::wclog.clear();

  if (!SetConsoleTitleW(console_window_title.data())) {
    std::wcerr << L"Console title was not set, error code: " << GetLastError()
               << "\n";
  }
}

gpu_renderer::Console::~Console() noexcept {
  if (!FreeConsole()) {
    OutputDebugStringW(L"Console wasn't freed because of error\n");
  }
}
