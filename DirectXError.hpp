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

#pragma warning(push)
#pragma warning(disable : 4514)
  [[nodiscard]] static constexpr bool OperationFailed(BOOL result) noexcept {
    return result == -1;
  };
#pragma warning(pop)

 private:
#pragma warning(push)
#pragma warning(disable : 4820)
  HRESULT error_code_{};
#pragma warning(pop)
};
}  // namespace gpu_renderer::exception

#endif  // !DIRECT_X_ERROR_HPP
