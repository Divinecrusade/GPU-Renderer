#include "Box.hpp"

namespace gpu_renderer::drawable {
Box::Box(Graphics& gfx) : Drawable{
          std::make_unique<bindable::IndexBuffer>(
              gfx, gsl::span<unsigned short const>{kIndicesForTriangle}),
        std::make_unique <bindable::VertexConstBuffer<DirectX::XMMATRIX>>(gfx)
} {
// GET BACK TO FULL CHILLI VARIANT OF BINDABLE
// THIS SHIT IS NOT GONNA TO WORK
}

void Box::Update(FrameTimer::DeltaTime dt,
                 DirectX::XMMATRIX const& projection) {
  cur_angle_ += dt * std::numbers::pi_v<float> / 4.f;
  while (cur_angle_ > 2.f * std::numbers::pi_v<float>)
    cur_angle_ -= 2.f * std::numbers::pi_v<float>;

  transformation_ = DirectX::XMMatrixTranspose(
      DirectX::XMMatrixRotationZ(cur_angle_) *
      DirectX::XMMatrixRotationX(cur_angle_) *
      DirectX::XMMatrixTranslation(0.f, 0.f, 3.f) * projection);

  vertex_const_buffer_.Update(transformation_);
}
}  // namespace gpu_renderer::drawable