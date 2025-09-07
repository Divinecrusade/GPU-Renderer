#ifndef DIRECT_X_ERROR_HPP
#define DIRECT_X_ERROR_HPP

#include "OptimisedStlHeader.hpp"
#include "OptimisedWindowsHeader.hpp"
#include "SystemError.hpp"

namespace gpu_renderer::exception {
class DirectXError : public SystemError {
 public:
  static constexpr std::wstring_view kTypeOfException{L"DirectX error"};

 public:
#ifdef _DEBUG
  DirectXError(wchar_t const* file, int line, char const* message,
               HRESULT operation_status);
#endif  // _DEBUG
  DirectXError(char const* message, HRESULT operation_status) noexcept;

  [[nodiscard]] int GetErrorCode() const noexcept override;
  [[nodiscard]] std::wstring_view GetTypeOfException() const noexcept override;
  [[nodiscard]] std::wstring WhatHappened() const override;

  [[nodiscard]] static constexpr bool OperationFailed(BOOL result) noexcept {
    return result == -1;
  };

 private:
  HRESULT error_code_{};
};
}  // namespace gpu_renderer::exception

#endif  // !DIRECT_X_ERROR_HPP
