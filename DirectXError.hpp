#ifndef DIRECT_X_ERROR_HPP
#define DIRECT_X_ERROR_HPP

#include "OptimisedStlHeader.hpp"
#include "OptimisedWindowsHeader.hpp"
#include "ErrorWithCode.hpp"
#include "DXDebugInfoManager.hpp"

namespace gpu_renderer::exception {
class DirectXError : public ErrorWithCode {
 public:
  static constexpr std::wstring_view kTypeOfException = L"DirectX error";

 public:
#ifdef _DEBUG
  DirectXError(wchar_t const* file, int line, char const* message,
               HRESULT operation_status) noexcept;
  DirectXError(wchar_t const* file, int line, char const* message,
               HRESULT operation_status,
               debug::DXDebugInfoManager const& debug_tracer);
#endif  // _DEBUG
  DirectXError(char const* message, HRESULT operation_status) noexcept;

  [[nodiscard]] int GetErrorCode() const noexcept override;
  [[nodiscard]] std::wstring_view GetTypeOfException() const noexcept override;
  [[nodiscard]] std::wstring WhatHappened() const override;

  [[nodiscard]] static constexpr bool OperationFailed(BOOL result) noexcept {
    return result == -1;
  };

  static DirectXError Create(HRESULT operation_status,
                             [[maybe_unused]] char const* message_debug,
                             [[maybe_unused]] char const* message_release,
                             [[maybe_unused]] wchar_t const* file,
                             [[maybe_unused]] int line) noexcept {
#ifdef _DEBUG
    return {file, line, message_debug, operation_status};
#else
    return {message_release, operation_status};
#endif  // _DEBUG
  }

 private:
  HRESULT error_code_{};
#ifdef _DEBUG
  std::optional<std::wstring> trace_log_ = std::nullopt;
#endif  // _DEBUG
};
}  // namespace gpu_renderer::exception

#endif  // !DIRECT_X_ERROR_HPP
