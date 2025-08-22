#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include "OptimisedWindowsHeader.hpp"
#include "OptimisedDirect3dHeader.hpp"
#include "OptimisedStlHeader.hpp"

namespace gpu_renderer {
class Graphics final {
 public:
  class Color final {
   private:
    static constexpr std::size_t kIndexComponentR{0u};
    static constexpr std::size_t kIndexComponentG{1u};
    static constexpr std::size_t kIndexComponentB{2u};

   public:
    Color(float r, float g, float b) noexcept;
    
    FLOAT const* operator&() const noexcept;

    [[nodiscard]] float GetR() const noexcept;
    [[nodiscard]] float GetG() const noexcept;
    [[nodiscard]] float GetB() const noexcept;
    
    void SetR(float r) noexcept;
    void SetG(float g) noexcept;
    void SetB(float b) noexcept;

   private:
    std::array<float, 4u> encoded_;
  };

 public:
  Graphics() = delete;
  Graphics(HWND hwnd);
  Graphics(Graphics const&) = delete;
  Graphics(Graphics&&) = delete;

  Graphics& operator=(Graphics const&) = delete;
  Graphics& operator=(Graphics&&) = delete;

  ~Graphics() noexcept;

  void EndFrame();

  void ClearBuffer(Color const& c);

 private:
  IDXGISwapChain* swap_chain_{};
  ID3D11Device* device_{};
  ID3D11DeviceContext* device_context_{};
  ID3D11RenderTargetView* target_{};
};
} // namespace gpu_renderer

#endif  // !GRAPHICS_HPP
