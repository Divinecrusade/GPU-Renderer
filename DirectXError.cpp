#include "DirectXError.hpp"

#include "OptimisedDirect3dHeader.hpp"
#include "OptimisedStlHeader.hpp"
#include "OptimisedGslHeader.hpp"

using namespace std::string_literals;

namespace gpu_renderer::exception {
#ifdef _DEBUG
DirectXError::DirectXError(wchar_t const* file, int line, char const* message,
                           HRESULT operation_status) noexcept
    : ErrorWithCode{file, line, message}, 
      error_code_{operation_status} {
  assert(((void)"Exception must be created from failed code status", 
          FAILED(operation_status)));
}

DirectXError::DirectXError(wchar_t const* file, int line, char const* message,
                           HRESULT operation_status,
                           debug::DXDebugInfoManager const& debug_tracer)
    : DirectXError{file, line, message, operation_status} {
    if (auto const expected_trace_result{debug_tracer.GetTraceLog()};
        expected_trace_result) {
      trace_log_ = (expected_trace_result->empty() ? L"No debug layer messages\n"s
                                                   : *expected_trace_result);
    }
    else {
      trace_log_ = expected_trace_result.error();
    }
    OutputDebugStringW(trace_log_->data());
    std::wclog << *trace_log_;
}
#endif  // _DEBUG

DirectXError::DirectXError(char const* message, HRESULT operation_status) noexcept
    : ErrorWithCode{message}, error_code_{operation_status} {
  assert(((void)"Exception must be created from failed code status",
          FAILED(operation_status)));
}

int DirectXError::GetErrorCode() const noexcept {
  return gsl::narrow<int>(error_code_);
}

std::wstring_view DirectXError::GetTypeOfException() const noexcept {
  return kTypeOfException;
}

std::wstring DirectXError::WhatHappened() const {
  auto const error_description = DXGetErrorStringW(error_code_);
#ifdef  _DEBUG
  if (trace_log_) {
    return FormatErrorMessage(std::format(L"{}\nTrace log:\n{}", 
                              error_description, *trace_log_));
  }
#endif  //  _DEBUG
  return FormatErrorMessage(error_description);
}
}  // namespace gpu_renderer::exception
