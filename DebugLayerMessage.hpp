#ifndef DEBUG_LAYER_MESSAGE_HPP
#define DEBUG_LAYER_MESSAGE_HPP

#include "OptimisedStlHeader.hpp"
#include "Error.hpp"

namespace gpu_renderer::exception {
class DebugLayerMessage : public Error {
 public:
  static constexpr std::wstring_view kTypeOfException = L"DirectX debug layer message";

 public:
  DebugLayerMessage(wchar_t const* file, int line, char const* message,
                    std::wstring trace_log) noexcept;

  [[nodiscard]] std::wstring_view GetTypeOfException() const noexcept override;
  [[nodiscard]] std::wstring WhatHappened() const override;

 private:
  std::wstring trace_log_{};
};
}  // namespace gpu_renderer::exception

#endif  // !DEBUG_LAYER_MESSAGE_HPP
