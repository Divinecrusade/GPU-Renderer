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
 public:
  SystemError() = default;
  SystemError(SystemError const&) = default;
  SystemError(SystemError&&) = default;

  SystemError& operator=(SystemError const&) = default;
  SystemError& operator=(SystemError&&) = default;

  virtual ~SystemError() = default;

 protected:
#ifdef _DEBUG
  SystemError(wchar_t const* file, int line, char const* message);
#endif  // _DEBUG
  SystemError(char const* message) noexcept;

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
#pragma warning(push)
#pragma warning(disable : 4820)
  std::filesystem::path file_{};
  int line_{};
#pragma warning(pop)
#endif  // _DEBUG
};
}  // namespace gpu_renderer::exception

#endif  // !SYSTEM_ERROR_HPP