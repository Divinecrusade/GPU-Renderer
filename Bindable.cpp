#include "Bindable.hpp"

namespace gpu_renderer {
ID3D11DeviceContext& Bindable::GetDeviceContext(
    Graphics const& gfx) noexcept {
  return *(gfx.device_context_.Get());
}

ID3D11Device& Bindable::GetDevice(Graphics const& gfx) noexcept {
  return *(gfx.device_.Get());
}

Graphics::ProtectiveLayer& Bindable::GetDebugger(
    Graphics const& gfx) noexcept {
  return gfx.debugger_;
}
}  // namespace gpu_renderer
