#include "Bindable.hpp"

#include "Graphics.hpp"

namespace gpu_renderer::abstract {
Bindable::Bindable(Graphics& gfx) : gfx_{gfx} {}

ID3D11DeviceContext& gpu_renderer::abstract::Bindable::GetDeviceContext() noexcept {
  return *(gfx_.device_context_.Get());
}

ID3D11Device& Bindable::GetDevice() noexcept {
  return *(gfx_.device_.Get());  
}
}  // namespace gpu_renderer::abstract
