#include "SystemError.hpp"

#include <cassert>
#include <format>

#include "OptimisedWindowsHeader.hpp"

namespace gpu_renderer::exception {
#ifdef _DEBUG
SystemError::SystemError(wchar_t const* file, int line, char const* message)
    : std::exception{message}, file_{file}, line_{line} {
  assert(((void)"File path cannot be nullptr", file != nullptr));
  assert(((void)"Line number must be positive", line > 0));
}

std::filesystem::path const& SystemError::InWhatFileThrowed() const noexcept {
  return file_;
}

int SystemError::InWhatLineOfCodeThrowed() const noexcept { return line_; }
#endif  // _DEBUG

SystemError::SystemError(char const* message) noexcept : exception{message} {}

std::wstring SystemError::FormatErrorMessage(
    std::wstring_view error_description) const {
  std::string const narrow_what{what()};
  std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};

#ifdef _DEBUG
  return std::format(
      L"[What] {}\n[Code] {}\n[Description] {}\n[File] {}\n[Line] {}",
      wide_what, GetErrorCode(), error_description, file_.wstring(), line_);
#else
  return std::format(L"[What] {}\n[Code] {}\n[Description] {}", wide_what,
                     GetErrorCode(), error_description);
#endif  // _DEBUG
}
}  // namespace gpu_renderer::exception