#include "WinError.hpp"

#include <cassert>
#include <format>
#include <iostream>

#ifdef _DEBUG
gpu_renderer::exception::WinError::WinError(wchar_t const* file, int line,
                                            char const* message,
                                            DWORD error_code)
    : std::exception{message},
      file_{file},
      line_{line},
      error_code_{error_code} {
  assert(((void)"URL of file with code where exception throwed cannot be empty",
          file != nullptr));
  assert(((void)"Line of code where exception throwed cannot be equal of less "
                "than zero",
          line > 0));
  assert(((void)"WinAPI error code cannot be 0", error_code_ != 0));
}

std::filesystem::path const&
gpu_renderer::exception::WinError::InWhatFileThrowed() const noexcept {
  return file_;
}

int gpu_renderer::exception::WinError::InWhatLineOfCodeThrowed()
    const noexcept {
  return line_;
}
#endif  // _DEBUG

gpu_renderer::exception::WinError::WinError(char const* message,
                                            DWORD error_code_) noexcept
    : exception{message}, error_code_{error_code_} {
  assert(((void)"WinAPI error code cannot be 0", error_code_ != NULL));
}

DWORD gpu_renderer::exception::WinError::GetErrorCode() const noexcept {
  return error_code_;
}

std::wstring gpu_renderer::exception::WinError::WhatHappened() const {
  LPWSTR lpMsgBuf{nullptr};

#pragma warning(push)
#pragma warning(disable : 26490)
  DWORD const format_result{FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error_code_, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&lpMsgBuf), 0, nullptr)};
#pragma warning(pop)

  std::wstring error_description{};
  if (bool const format_succeded{format_result != 0 && lpMsgBuf != nullptr};
      format_succeded) {
    error_description = lpMsgBuf;
#ifdef _DEBUG
    if (bool const free_failed{LocalFree(lpMsgBuf) != NULL}; free_failed) {
      try {
        std::wcerr << "Error happened during free FormatMessage buffer, code: "
                   << GetLastError() << "\n";
      } catch (...) {
        OutputDebugStringW(L"Exception raised in log WinError WhatHappened");
      }
    }
#else
    (void)LocalFree(lpMsgBuf);
#endif  // _DEBUG
  } else {
    error_description = L"Unknown error";
  }

  std::string const narrow_what{what()};
  std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};

#ifdef _DEBUG
  return std::format(
      L"[What] {}\n[Code] 0x{:08X}\n[Description] {}\n[File] {}\n[Line] {}",
      wide_what, static_cast<unsigned int>(error_code_), error_description,
      file_.wstring(), line_);
#else
  return std::format(L"[What] {}\n[Code] 0x{:08X}\n[Description] {}", wide_what,
                     static_cast<unsigned int>(error_code_), error_description);
#endif  // _DEBUG
}
