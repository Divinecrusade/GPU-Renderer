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
           int error_code);
#endif  // _DEBUG
  CrtError(char const* message, int error_code_) noexcept;

  [[nodiscard]] int GetErrorCode() const noexcept override;
  [[nodiscard]] std::wstring_view GetTypeOfException() const noexcept override;
  [[nodiscard]] std::wstring WhatHappened() const override;

 private:
#pragma warning(push)
#pragma warning(disable : 4820)
  int error_code_{};
#pragma warning(pop)
};
}  // namespace gpu_renderer::exception

#endif  // !CRT_ERROR_HPP
