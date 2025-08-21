#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include "OptimisedWindowsHeader.hpp"
#include "OptimisedDirect3dHeader.hpp"

namespace gpu_renderer {
class Graphics final {
 public:
  Graphics() = delete;
  Graphics(HWND hwnd) noexcept;
  Graphics(Graphics const&) = delete;
  Graphics(Graphics&&) = delete;

  Graphics& operator=(Graphics const&) = delete;
  Graphics& operator=(Graphics&&) = delete;

  ~Graphics() noexcept;

  void EndFrame();

  void ClearBuffer(float r, float g, float b);

 private:
  IDXGISwapChain* swap_chain_{};
  ID3D11Device* device_{};
  ID3D11DeviceContext* device_context_{};
  ID3D11RenderTargetView* target_{};
};
} // namespace gpu_renderer

#endif  // !GRAPHICS_HPP
