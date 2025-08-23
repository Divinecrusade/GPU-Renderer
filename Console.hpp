#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include "OptimisedStlHeader.hpp"

namespace gpu_renderer::debug {
class Console final {
 public:
  static void InitStdStreams(std::wstring_view console_window_title);

 private:
  Console() = delete;
  Console(std::wstring_view console_window_title);
  Console(Console const&) = delete;
  Console(Console&&) = delete;

  Console& operator=(Console const&) = delete;
  Console& operator=(Console&&) = delete;

  ~Console() noexcept;
};
}  // namespace gpu_renderer::debug

#endif  // !CONSOLE_HPP
