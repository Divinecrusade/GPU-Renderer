#ifndef WIN_ERROR_HPP
#define WIN_ERROR_HPP

#include <exception>
#ifdef _DEBUG
#include <filesystem>
#endif  // _DEBUG
#include <string>
#include <string_view>

#include "OptimisedWindowsHeader.hpp"

namespace gpu_renderer::exception {
class WinError : public std::exception {
 public:
  static constexpr std::wstring_view kTypeOfException{L"WinAPI error"};

 public:
#ifdef _DEBUG
  WinError(wchar_t const* file, int line, char const* message,
           DWORD error_code);
  [[nodiscard]] std::filesystem::path const& InWhatFileThrowed() const noexcept;
  [[nodiscard]] int InWhatLineOfCodeThrowed() const noexcept;
#endif  // _DEBUG
  WinError(char const* message, DWORD error_code_) noexcept;

  [[nodiscard]] DWORD GetErrorCode() const noexcept;
  [[nodiscard]] std::wstring WhatHappened() const;

 private:
#ifdef _DEBUG
  std::filesystem::path file_{};
  int line_{};
#endif  // _DEBUG

  DWORD error_code_{};
};
}  // namespace gpu_renderer::exception

#endif  // !WIN_ERROR_HPP
