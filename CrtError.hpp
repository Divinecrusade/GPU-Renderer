#ifndef CRT_ERROR_HPP
#define CRT_ERROR_HPP

#include <corecrt.h>

#include "SystemError.hpp"

namespace gpu_renderer::exception {
class CrtError : public SystemError {
 public:
  static constexpr std::wstring_view kTypeOfException{L"C runtime library (CRT) error"};

 public:
#ifdef _DEBUG
  CrtError(wchar_t const* file, int line, char const* message,
           errno_t error_code);
#endif  // _DEBUG
  CrtError(char const* message, errno_t error_code_) noexcept;

  [[nodiscard]] int GetErrorCode() const noexcept override;
  [[nodiscard]] std::wstring_view GetTypeOfException() const noexcept override;
  [[nodiscard]] std::wstring WhatHappened() const override;

  [[nodiscard]] constexpr static bool OperationFailed(errno_t operation_result) noexcept {
    return operation_result != 0;
  }

  static CrtError Create(errno_t error_code, 
                         [[maybe_unused]] char const* message_debug,
                         [[maybe_unused]] char const* message_release,
                         [[maybe_unused]] wchar_t const* file,
                         [[maybe_unused]] int line) {
#ifdef _DEBUG
    return {file, line, message_debug, error_code};
#else
    return {message_release, error_code};
#endif  // _DEBUG
  }

 private:
  errno_t error_code_{};
};
}  // namespace gpu_renderer::exception

#endif  // !CRT_ERROR_HPP
