#ifndef BINDABLE_HPP
#define BINDABLE_HPP

#include "OptimisedDirect3dHeader.hpp"

namespace gpu_renderer {
class Graphics;

namespace abstract {
class Bindable {
 public:
  
  Bindable(Graphics& gfx);
  virtual ~Bindable() = default;
  
  virtual void Activate() = 0;

 protected:
  ID3D11DeviceContext& GetDeviceContext() noexcept;
  ID3D11Device& GetDevice() noexcept;

 private:
  Graphics const& gfx_;
};
}  // namespace pgu_renderer::abstract
}  // namespace pgu_renderer

#endif  // !BINDABLE_HPP
