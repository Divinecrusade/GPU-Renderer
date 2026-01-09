#ifndef BINDABLE_HPP
#define BINDABLE_HPP

#include "Graphics.hpp"

namespace gpu_renderer {

namespace abstract {
class Bindable {
 public:
  
  Bindable(Graphics& gfx);
  virtual ~Bindable() = default;
  
  virtual void Activate() = 0;

 protected:
  ID3D11DeviceContext& GetDeviceContext() noexcept;
  ID3D11Device& GetDevice() noexcept;
  Graphics::ProtectiveLayer& GetDebugger() noexcept;

 private:
  Graphics const& gfx_;
};
}  // namespace pgu_renderer::abstract
}  // namespace pgu_renderer

#endif  // !BINDABLE_HPP
