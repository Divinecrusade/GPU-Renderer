#ifndef CACHED_DC_HPP
#define CACHED_DC_HPP

#include "WindowClass.hpp"

namespace gpu_renderer {
class CachedDC : public WindowClass {
 public:
  static constexpr LPCWSTR CLASS_NAME{L"Cached DC"}; 

 public:
  CachedDC() = delete;
  CachedDC(HINSTANCE hInstance, WNDPROC lpfnWndProc) noexcept;
  CachedDC(CachedDC const&) = delete;
  CachedDC(CachedDC&&) = delete;

  CachedDC& operator=(CachedDC const&) = delete;
  CachedDC& operator=(CachedDC&&) = delete;
};
}

#endif  // !CACHED_DC_HPP
