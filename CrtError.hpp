#ifndef CRT_ERROR
#define CRT_ERROR

#include <exception>
#ifdef _DEBUG
#include <filesystem>
#endif  // _DEBUG
#include <string>
#include <string_view>
#include <corecrt.h>

namespace gpu_renderer::exception {
class CrtError : public std::exception {
 public:
  static constexpr std::wstring_view kTypeOfException{L"C runtime library (CRT) error"};

 public:
#ifdef _DEBUG
  CrtError(wchar_t const* file, int line, char const* message,
           int error_code);
  [[nodiscard]] std::filesystem::path const& InWhatFileThrowed() const noexcept;
  [[nodiscard]] int InWhatLineOfCodeThrowed() const noexcept;
#endif  // _DEBUG
  CrtError(char const* message, int error_code_) noexcept;

  [[nodiscard]] int GetErrorCode() const noexcept;
  [[nodiscard]] std::wstring WhatHappened() const;

 private:
#ifdef _DEBUG
  std::filesystem::path file_{};
  int line_{};
#endif  // _DEBUG

  int error_code_{};
};
}  // namespace gpu_renderer::exception

#endif  // !CRT_ERROR
