#ifndef ERROR_WITH_CODE_HPP
#define ERROR_WITH_CODE_HPP

#include "Error.hpp"

namespace gpu_renderer::exception {
class ErrorWithCode : public Error {
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

 public:
  [[nodiscard]] virtual int GetErrorCode() const noexcept = 0;

 protected:
  [[nodiscard]] std::wstring FormatErrorMessage(
      std::wstring_view error_description) const override;
};
}  // namespace gpu_renderer::exception

#endif  // !SYSTEM_ERROR_HPP
