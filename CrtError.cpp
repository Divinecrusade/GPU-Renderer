#include "CrtError.hpp"

#include <cassert>
#include <iostream>

#include "OptimisedWindowsHeader.hpp"

namespace gpu_renderer::exception {
#ifdef _DEBUG
CrtError::CrtError(wchar_t const* file, int line, char const* message,
                   int error_code)
    : SystemError{file, line, message},
      error_code_{error_code} {
  assert(((void)"CRT error code cannot be 0", error_code_ != 0));
}
#endif  // _DEBUG

CrtError::CrtError(char const* message, int error_code) noexcept
    : SystemError{message}, error_code_{error_code} {
  assert(((void)"CRT error code cannot be 0", error_code_ != 0));
}

int CrtError::GetErrorCode() const noexcept { return error_code_; }

std::wstring_view CrtError::GetTypeOfException() const noexcept {
  return kTypeOfException;
}

std::wstring CrtError::WhatHappened() const {
  static constexpr std::size_t kMaxErrorMessageLength{256ull};
  static constexpr auto FormatFailed = [](errno_t format_result) {
    return format_result != 0;
  };
  std::wstring error_description('\0', kMaxErrorMessageLength);
  
  errno_t const format_result{
      _wcserror_s(error_description.data(), 
                  sizeof(wchar_t) * kMaxErrorMessageLength, error_code_)};

  if (FormatFailed(format_result)) {
    error_description = L"Unknown CRT error";
#ifdef _DEBUG
    try {
      std::wcerr << L"strerror_s failed with error code: " << format_result
                 << L" for errno: " << error_code_ << L"\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log CrtError WhatHappened\n");
    }
#endif  // _DEBUG
  }
  error_description.shrink_to_fit();
  return FormatErrorMessage(error_description);
}
}  // namespace gpu_renderer::exception