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
#include "CrtError.hpp"

namespace gpu_renderer::debug {
void Console::InitStdStreams(std::wstring_view console_window_title) {
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

Console::Console(std::wstring_view console_window_title) {
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
#ifdef _DEBUG
    throw exception::CrtError{__FILEW__, __LINE__, "stdout setmode failed",
                              errno};
#else
    throw exception::CrtError{"Console (out and log streams) code page is not set correctly", 
                              errno};
#endif  // _DEBUG
  }
  if (SetModeFailed(_setmode(_fileno(stderr), _O_U8TEXT))) {
#ifdef _DEBUG
    throw exception::CrtError{__FILEW__, __LINE__,
                              "stderr setmode failed", 
                              errno};
#else
    throw exception::CrtError{"Console (err stream) code page is not set correctly",
                              errno};
#endif  // _DEBUG
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

Console::~Console() noexcept {
  if (!FreeConsole()) {
    OutputDebugStringW(L"Console wasn't freed because of error\n");
  }
}
}  // namespace gpu_renderer::debug
