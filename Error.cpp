#include "Error.hpp"

namespace gpu_renderer::exception {
#ifdef _DEBUG
Error::Error(wchar_t const* file, int line,
             char const* message) noexcept
    : std::exception{message}, file_{file}, line_{line} {
  assert(((void)"File path cannot be nullptr", file != nullptr));
  assert(((void)"Line number must be positive", line > 0));
}

std::filesystem::path Error::InWhatFileThrowed() const { return file_; }

int Error::InWhatLineOfCodeThrowed() const noexcept { return line_; }
#endif  // _DEBUG

Error::Error(char const* message) noexcept : exception{message} {}

std::wstring Error::FormatErrorMessage(std::wstring_view error_description) const {
  std::string const narrow_what = what();
  std::wstring const wide_what{narrow_what.begin(), narrow_what.end()};

#ifdef _DEBUG
  return std::format(L"[What] {}\n[Description]\n {}\n[File] {}\n[Line] {}",
                     wide_what, error_description, file_, line_);
#else
  return std::format(L"[What] {}\n[Description]\n {}", 
                     wide_what, error_description);
#endif  // _DEBUG
}
}  // gpu_renderer::exception
