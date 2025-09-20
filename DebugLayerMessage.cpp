#include "DebugLayerMessage.hpp"

namespace gpu_renderer::exception {
DebugLayerMessage::DebugLayerMessage(wchar_t const* file, int line, 
                                     char const* message,
                                     std::wstring trace_log)
    : Error{file, line, message}, 
      trace_log_{std::move(trace_log)} {}

std::wstring_view DebugLayerMessage::GetTypeOfException() const noexcept {
  return kTypeOfException;
}

std::wstring DebugLayerMessage::WhatHappened() const { 
  return FormatErrorMessage(trace_log_);
}
}  // namespace gpu_renderer::exception

