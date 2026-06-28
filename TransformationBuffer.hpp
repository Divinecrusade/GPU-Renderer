#ifndef TRANSFORMATION_BUFFER
#define TRANSFORMATION_BUFFER

#include "Bindable.hpp"
#include "Drawable.hpp"
#include "ConstBuffer.hpp"

namespace gpu_renderer::bindable {
class TransformationBuffer : public Bindable {
 public:
  TransformationBuffer(Graphics& gfx, Drawable& parent);

  void Bind(Graphics& gfx);

 private:
  Drawable& parent_;
  VertexConstBuffer<DirectX::XMMATRIX> vertex_const_buffer_;
};
}

#endif  // !TRANSFORMATION_CONST_BUFFER
