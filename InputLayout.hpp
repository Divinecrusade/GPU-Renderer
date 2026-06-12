#ifndef INPUT_LAYOUT_HPP
#define INPUT_LAYOUT_HPP

#include "Shader.hpp"

namespace gpu_renderer::bindable {
class InputLayout : public Graphics::Bindable {
 public:
  InputLayout(VertexShader const& vertex_shader);

  void Bind(Graphics& gfx) override;

 private:
  VertexShader const& vertex_shader_;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout_{};
};
}  // namespace gpu_renderer::bindable

#endif  // !INPUT_LAYOUT_HPP
