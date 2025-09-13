#ifndef WIN_ERROR_HPP
#define WIN_ERROR_HPP

#include "OptimisedStlHeader.hpp"
#include "OptimisedWindowsHeader.hpp"
#include "SystemError.hpp"

namespace gpu_renderer::exception {
class WinError : public SystemError {
 public:
  static constexpr std::wstring_view kTypeOfException = L"WinAPI error";

 public:
#ifdef _DEBUG
  WinError(wchar_t const* file, int line, char const* message,
           DWORD error_code) noexcept;
#endif  // _DEBUG
  WinError(char const* message, DWORD error_code_) noexcept;

  [[nodiscard]] int GetErrorCode() const noexcept override;
  [[nodiscard]] std::wstring_view GetTypeOfException() const noexcept override;
  [[nodiscard]] std::wstring WhatHappened() const override;

  [[nodiscard]] static constexpr bool OperationFailed(BOOL result) noexcept {
    return result == -1;
  };

  static WinError CreateFromGetLastError([[maybe_unused]] char const* message_debug,
                                         [[maybe_unused]] char const* message_release,
                                         [[maybe_unused]] wchar_t const* file,
                                         [[maybe_unused]] int line) noexcept {
#ifdef _DEBUG
    return {file, line, message_debug, GetLastError()};
#else
    return {message_release, GetLastError()};
#endif  // _DEBUG
  }

 private:
  DWORD error_code_{};
};
}  // namespace gpu_renderer::exception

#endif  // !WIN_ERROR_HPP
