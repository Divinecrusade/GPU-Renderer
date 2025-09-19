#include "WinError.hpp"

#include "OptimisedGslheader.hpp"
#include "OptimisedStlHeader.hpp"

namespace gpu_renderer::exception {
#ifdef _DEBUG
WinError::WinError(wchar_t const* file, int line, char const* message,
                   DWORD error_code) noexcept
    : ErrorWithCode{file, line, message},
      error_code_{error_code} {
  assert(((void)"WinAPI error code cannot be 0", error_code_ != 0));
}
#endif  // _DEBUG

WinError::WinError(char const* message, DWORD error_code_) noexcept
    : ErrorWithCode{message}, error_code_{error_code_} {
  assert(((void)"WinAPI error code cannot be 0", error_code_ != 0));
}

int WinError::GetErrorCode() const noexcept {
  return gsl::narrow<int>(error_code_);
}

std::wstring_view WinError::GetTypeOfException() const noexcept {
  return kTypeOfException;
}

std::wstring WinError::WhatHappened() const {
  LPWSTR lpMsgBuf{nullptr};

#pragma warning(push)
#pragma warning(disable : 26490)
  DWORD const format_result = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error_code_, 
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&lpMsgBuf), 0, nullptr);
#pragma warning(pop)

  std::wstring error_description{};
  if (bool const format_succeded = (format_result != 0 && lpMsgBuf != nullptr);
      format_succeded) {
    error_description = lpMsgBuf;
#ifdef _DEBUG
    if (!LocalFree(lpMsgBuf)) {
      try {
        OutputDebugStringW(L"Error happened during free FormatMessage buffer\n");
        std::wcerr << L"Error happened during free FormatMessage buffer, code: "
                   << GetLastError() << "\n";
      } 
      catch (...) {
        OutputDebugStringW(L"Exception raised in log WinError WhatHappened");
      }
    }
#else
    std::ignore = LocalFree(lpMsgBuf);
#endif  // _DEBUG
  } 
  else {
    error_description = L"Unknown error";
  }

  return FormatErrorMessage(error_description);
}
}  // namespace gpu_renderer::exception
