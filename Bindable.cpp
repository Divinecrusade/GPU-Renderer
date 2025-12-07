#include "Bindable.hpp"

#include "Graphics.hpp"

namespace gpu_renderer::abstract {
ID3D11DeviceContext& gpu_renderer::abstract::Bindable::GetDeviceContext(
    Graphics const& gfx) noexcept {
    return *gfx.device_context_.Get();
}

ID3D11Device& Bindable::GetDevice(Graphics const& gfx) noexcept {
  return *gfx.device_.Get();  
}
}  // namespace gpu_renderer::abstract
