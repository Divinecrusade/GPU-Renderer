#ifndef BINDABLE_HPP
#define BINDABLE_HPP

#include "OptimisedDirect3dHeader.hpp"

namespace gpu_renderer {
class Graphics;

namespace abstract {
class Bindable {
 public:
  virtual void Bind(gpu_renderer::Graphics& gfx) = 0;
  virtual ~Bindable() = default;

 protected:
  static [[nodiscard]] ID3D11DeviceContext& GetDeviceContext(Graphics const& gfx) noexcept;
  static [[nodiscard]] ID3D11Device& GetDevice(Graphics const& gfx) noexcept;
};
}  // namespace pgu_renderer::abstract
}  // namespace pgu_renderer

#endif  // !BINDABLE_HPP
