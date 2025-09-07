#include "DirectXError.hpp"

#include "OptimisedDirect3dHeader.hpp"
#include "OptimisedStlHeader.hpp"
#include "OptimisedGslHeader.hpp"

namespace gpu_renderer::exception {
#ifdef _DEBUG
DirectXError::DirectXError(wchar_t const* file, int line, char const* message,
                           HRESULT operation_status)
    : SystemError{file, line, message}, 
      error_code_{operation_status} {
  assert(FAILED(operation_status));
}
#endif  // _DEBUG

DirectXError::DirectXError(char const* message, HRESULT operation_status) noexcept
    : SystemError{message}, error_code_{operation_status} {
  assert(FAILED(operation_status));
}

int DirectXError::GetErrorCode() const noexcept {
  return gsl::narrow<int>(error_code_);
}

std::wstring_view DirectXError::GetTypeOfException() const noexcept {
  return kTypeOfException;
}

std::wstring DirectXError::WhatHappened() const {
  return FormatErrorMessage(DXGetErrorStringW(error_code_));
}
}  // namespace gpu_renderer::exception
