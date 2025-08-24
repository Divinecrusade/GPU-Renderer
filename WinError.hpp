#ifndef WIN_ERROR_HPP
#define WIN_ERROR_HPP

#include "OptimisedStlHeader.hpp"
#include "OptimisedWindowsHeader.hpp"
#include "SystemError.hpp"

namespace gpu_renderer::exception {
class WinError : public SystemError {
 public:
  static constexpr std::wstring_view kTypeOfException{L"WinAPI error"};

 public:
#ifdef _DEBUG
  WinError(wchar_t const* file, int line, char const* message,
           DWORD error_code);
#endif  // _DEBUG
  WinError(char const* message, DWORD error_code_) noexcept;

  [[nodiscard]] int GetErrorCode() const noexcept override;
  [[nodiscard]] std::wstring_view GetTypeOfException() const noexcept override;
  [[nodiscard]] std::wstring WhatHappened() const override;

#pragma warning(push)
#pragma warning(disable : 4514)
  [[nodiscard]] static constexpr bool OperationFailed(BOOL result) noexcept {
    return result == -1;
  };
#pragma warning(pop)

 private:
#pragma warning(push)
#pragma warning(disable : 4820)
  DWORD error_code_{};
#pragma warning(pop)
};
}  // namespace gpu_renderer::exception

#endif  // !WIN_ERROR_HPP
