#include "Console.hpp"

#include <fcntl.h>
#include <io.h>

#include "OptimisedStlHeader.hpp"
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
  std_streams_initialized = true;
#endif  // _DEBUG
}

Console::Console(std::wstring_view console_window_title) {
  if (!AllocConsole()) [[unlikely]] {
    throw exception::WinError::CreateFromGetLastError(
        "Console wasn't allocated",
        "Console wasn't allocated",
        __FILEW__, __LINE__);
  }

  FILE* cout_stream{nullptr};
  if (errno_t const operation_code_result{freopen_s(&cout_stream, "CONOUT$", "w", stdout)};
      exception::CrtError::OperationFailed(operation_code_result))
      [[unlikely]] {
#ifdef _DEBUG
    throw exception::CrtError{__FILEW__, __LINE__, "Cout stream wasn't reopened",
                              operation_code_result};
#else
    throw exception::CrtError{"Console wasn't opened",
                              operation_code_result};
#endif  // _DEBUG
  }
  assert(((void)"Cout stream must be initialised", cout_stream != nullptr));
  __assume(cout_stream != nullptr);

  FILE* cerr_stream{nullptr};
  if (errno_t const operation_code_result{freopen_s(&cerr_stream, "CONOUT$", "w", stderr)};
      exception::CrtError::OperationFailed(operation_code_result))
      [[unlikely]] {
#ifdef _DEBUG
    throw exception::CrtError{__FILEW__, __LINE__,
                              "Cerr stream wasn't reopened",
                              operation_code_result};
#else
    throw exception::CrtError{"Console wasn't opened", operation_code_result};
#endif  // _DEBUG
  }
  assert(((void)"Cout stream must be initialised", cerr_stream != nullptr));
  __assume(cerr_stream != nullptr);

  FILE* cin_stream{nullptr};
  if (errno_t const operation_code_result{freopen_s(&cin_stream, "CONIN$", "r", stdin)};
      exception::CrtError::OperationFailed(operation_code_result))
      [[unlikely]] {
#ifdef _DEBUG
    throw exception::CrtError{__FILEW__, __LINE__,
                              "Cin stream wasn't reopened",
                              operation_code_result};
#else
    throw exception::CrtError{"Console wasn't opened", operation_code_result};
#endif  // _DEBUG
  }
  assert(((void)"Cin stream must be initialised", cin_stream != nullptr));
  __assume(cin_stream != nullptr);

  static constexpr UINT kCpUnicode{65001u};
  if (!SetConsoleOutputCP(kCpUnicode)) [[unlikely]] {
    throw exception::WinError::CreateFromGetLastError(
        "Console output code page was not set to Unicode",
        "Console code page was not properly set",
        __FILEW__, __LINE__);
  }
  if (!SetConsoleCP(kCpUnicode)) [[unlikely]] {
    throw exception::WinError::CreateFromGetLastError(
        "Console input code page was not set to Unicode",
        "Console code page was not properly set",
        __FILEW__, __LINE__);
  }

  static constexpr auto SetModeFailed = [](auto op_status) {
    return op_status < 0;
  };
  if (SetModeFailed(_setmode(_fileno(stdout), _O_U8TEXT))) [[unlikely]] {
#ifdef _DEBUG
    throw exception::CrtError{__FILEW__, __LINE__, "stdout setmode failed",
                              errno};
#else
    throw exception::CrtError{"Console (out and log streams) code page is not set correctly", 
                              errno};
#endif  // _DEBUG
  }
  if (SetModeFailed(_setmode(_fileno(stderr), _O_U8TEXT))) [[unlikely]] {
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

  if (!SetConsoleTitleW(console_window_title.data())) [[unlikely]] {
    std::wcerr << L"Console title was not set, error code: " << GetLastError()
               << "\n";
  }
}

Console::~Console() noexcept {
  if (!FreeConsole()) [[unlikely]] {
    OutputDebugStringW(L"Console wasn't freed because of error\n");
  }
}
}  // namespace gpu_renderer::debug
