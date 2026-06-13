#ifndef TOPOLOGY_HPP
#define TOPOLOGY_HPP

#include "Bindable.hpp"
#include "SupportedTopologyType.hpp"

namespace gpu_renderer::bindable {
template <SupportedTypologyType T>
class Topology : public Bindable {
 public:
  void Bind(Graphics& gfx) override {
    GetDeviceContext(gfx).IASetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(T));
  }
};
}  // namespace gpu_renderer::bindable

#endif  // !TOPOLOGY_HPP
