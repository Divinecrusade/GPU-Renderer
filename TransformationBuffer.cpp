#include "TransformationBuffer.hpp"

namespace gpu_renderer::bindable {
TransformationBuffer::TransformationBuffer(Graphics& gfx, Drawable& parent)
    :
parent_{parent},
vertex_const_buffer_{gfx} {}

void TransformationBuffer::Bind(Graphics& gfx)
{
  vertex_const_buffer_.Update(gfx, DirectX::XMMatrixTranspose(parent_.GetTransformation() * gfx.GetProjection()));
  vertex_const_buffer_.Bind(gfx);
}
}