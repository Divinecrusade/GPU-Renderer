#ifndef CACHED_DC_HPP
#define CACHED_DC_HPP

#include "WindowClass.hpp"

namespace gpu_renderer {
class CachedDC : public WindowClass {
 private:
  static constexpr LPCWSTR kClassName{L"Cached DC"};

 public:
  CachedDC() = delete;
  CachedDC(HINSTANCE hInstance, WNDPROC lpfnWndProc);
  CachedDC(CachedDC const&) = delete;
  CachedDC(CachedDC&&) = delete;

  CachedDC& operator=(CachedDC const&) = delete;
  CachedDC& operator=(CachedDC&&) = delete;

  ~CachedDC() = default;
};
}  // namespace gpu_renderer

#endif  // !CACHED_DC_HPP
