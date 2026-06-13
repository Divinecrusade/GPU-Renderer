#ifndef INPUT_LAYOUT_HPP
#define INPUT_LAYOUT_HPP

#include "Bindable.hpp"
#include "Shader.hpp"

namespace gpu_renderer::bindable {
class InputLayout : public Bindable {
 public:
  InputLayout(Graphics& gfx, VertexShader const& vertex_shader);

  void Bind(Graphics& gfx) override;

 private:
  Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout_{};
};
}  // namespace gpu_renderer::bindable

#endif  // !INPUT_LAYOUT_HPP
