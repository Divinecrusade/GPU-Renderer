#ifndef SYSTEM_ERROR_HPP
#define SYSTEM_ERROR_HPP

#include <exception>
#ifdef _DEBUG
#include <filesystem>
#endif  // _DEBUG
#include <string>
#include <string_view>

namespace gpu_renderer::exception {
class SystemError : public std::exception {
 protected:
#ifdef _DEBUG
  SystemError(wchar_t const* file, int line, char const* message);
#endif  // _DEBUG
  SystemError(char const* message) noexcept;

  virtual ~SystemError() = default;

#ifdef _DEBUG
  [[nodiscard]] std::filesystem::path const& InWhatFileThrowed() const noexcept;
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
  std::filesystem::path file_{};
  int line_{};
#endif  // _DEBUG
};
}  // namespace gpu_renderer::exception

#endif  // !SYSTEM_ERROR_HPP