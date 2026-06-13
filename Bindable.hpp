#ifndef BINDABLE_HPP
#define BINDABLE_HPP

#include "Graphics.hpp"

namespace gpu_renderer {
class Bindable {
 public:
  virtual void Bind(Graphics& gfx) = 0;
  virtual ~Bindable() = default;

 protected:
  ID3D11DeviceContext& GetDeviceContext(Graphics const& gfx) noexcept;
  ID3D11Device& GetDevice(Graphics const& gfx) noexcept;
  Graphics::ProtectiveLayer& GetDebugger(Graphics const& gfx) noexcept;
};
}  // namespace gpu_renderer

#endif  // !BINDABLE_HPP
