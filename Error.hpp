#ifndef ERROR_HPP
#define ERROR_HPP

#include "OptimisedStlHeader.hpp"

namespace gpu_renderer::exception {
class Error : public std::exception {
 public:
  Error() = default;
  Error(Error const&) = default;
  Error(Error&&) = default;

  Error& operator=(Error const&) = default;
  Error& operator=(Error&&) = default;

  virtual ~Error() = default;

 protected:
#ifdef _DEBUG
  Error(wchar_t const* file, int line, char const* message) noexcept;
#endif  // _DEBUG
  Error(char const* message) noexcept;

#ifdef _DEBUG
  [[nodiscard]] std::filesystem::path InWhatFileThrowed() const;
  [[nodiscard]] int InWhatLineOfCodeThrowed() const noexcept;
#endif  // _DEBUG

 public:
  [[nodiscard]] virtual std::wstring_view GetTypeOfException() const noexcept = 0;
  [[nodiscard]] virtual std::wstring WhatHappened() const = 0;

 protected:
  [[nodiscard]] virtual std::wstring FormatErrorMessage(
      std::wstring_view error_description) const;

 private:
#ifdef _DEBUG
  wchar_t const* file_{};
  int line_{};
#endif  // _DEBUG
};
}  // namespace gpu_renderer::exception

#endif  // !ERROR_HPP
