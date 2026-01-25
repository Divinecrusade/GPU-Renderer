#ifndef TOPOLOGY_HPP
#define TOPOLOGY_HPP

#include "Graphics.hpp"
#include "SupportedTopologyType.hpp"

namespace gpu_renderer::bindable {
template <SupportedTypologyType T>
class Topology : public Graphics::Bindable {
 public:
  Topology(Graphics& gfx) : Bindable{gfx} {}
  
  void Activate() override { 
    GetDeviceContext().IASetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(T));
  }
};
}  // namespace gpu_renderer::bindable

#endif  // !TOPOLOGY_HPP
