#include "ErrorWithCode.hpp"

#include "OptimisedWindowsHeader.hpp"

namespace gpu_renderer::exception {
#ifdef _DEBUG
ErrorWithCode::ErrorWithCode(wchar_t const* file, int line, char const* message) noexcept
    : Error{file, line, message} {}
#endif  // _DEBUG

ErrorWithCode::ErrorWithCode(char const* message) noexcept : Error{message} {}

std::wstring ErrorWithCode::FormatErrorMessage(std::wstring_view error_description) const {
  return std::format(L"[Code] {}\n{}", GetErrorCode(), 
                     Error::FormatErrorMessage(error_description));
}
}  // namespace gpu_renderer::exception
