#include "CrtError.hpp"

#include <cassert>
#include <array>
#include <format>
#include <iostream>
#include <gsl/gsl>

#include "OptimisedWindowsHeader.hpp"

namespace gpu_renderer::exception {
#ifdef _DEBUG
CrtError::CrtError(wchar_t const* file, int line, char const* message,
                   int error_code)
    : std::exception{message},
      file_{file},
      line_{line},
      error_code_{error_code} {
  assert(((void)"File path cannot be nullptr", file != nullptr));
  assert(((void)"Line number must be positive", line > 0));
  assert(((void)"CRT error code cannot be 0", error_code_ != 0));
}

std::filesystem::path const& CrtError::InWhatFileThrowed() const noexcept {
  return file_;
}

int CrtError::InWhatLineOfCodeThrowed() const noexcept { return line_; }
#endif  // _DEBUG

CrtError::CrtError(char const* message, int error_code) noexcept
    : exception{message}, error_code_{error_code} {
  assert(((void)"CRT error code cannot be 0", error_code_ != 0));
}

int CrtError::GetErrorCode() const noexcept { return error_code_; }

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
  std::string const narrow_what{what()};
  std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};

#ifdef _DEBUG
  return std::format(
      L"[What] {}\n[Code] {}\n[Description] {}\n[File] {}\n[Line] {}",
      wide_what, error_code_, error_description, file_.wstring(), line_);
#else
  return std::format(L"[What] {}\n[Code] {}\n[Description] {}", wide_what,
                     error_code_, error_description);
#endif  // _DEBUG
}
}  // namespace gpu_renderer::exception