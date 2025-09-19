#ifndef ERROR_WITH_CODE_HPP
#define ERROR_WITH_CODE_HPP

#include "OptimisedStlHeader.hpp"

namespace gpu_renderer::exception {
class ErrorWithCode : public std::exception {
 public:
  ErrorWithCode() = default;
  ErrorWithCode(ErrorWithCode const&) = default;
  ErrorWithCode(ErrorWithCode&&) = default;

  ErrorWithCode& operator=(ErrorWithCode const&) = default;
  ErrorWithCode& operator=(ErrorWithCode&&) = default;

  virtual ~ErrorWithCode() = default;

 protected:
#ifdef _DEBUG
  ErrorWithCode(wchar_t const* file, int line, char const* message) noexcept;
#endif  // _DEBUG
  ErrorWithCode(char const* message) noexcept;

#ifdef _DEBUG
  [[nodiscard]] std::filesystem::path InWhatFileThrowed() const;
  [[nodiscard]] int InWhatLineOfCodeThrowed() const noexcept;
#endif  // _DEBUG

 public:
  [[nodiscard]] virtual int GetErrorCode() const noexcept = 0;
  [[nodiscard]] virtual std::wstring_view GetTypeOfException()
      const noexcept = 0;
  [[nodiscard]] virtual std::wstring WhatHappened() const = 0;

 protected:
  [[nodiscard]] std::wstring FormatErrorMessage(
      std::wstring_view error_description) const;

 private:
#ifdef _DEBUG
  wchar_t const* file_{};
  int line_{};
#endif  // _DEBUG
};
}  // namespace gpu_renderer::exception

#endif  // !SYSTEM_ERROR_HPP
