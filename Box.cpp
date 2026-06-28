#include "Box.hpp"

#include "ConstBuffer.hpp"
#include "IndexBuffer.hpp"
#include "InputLayout.hpp"
#include "Shader.hpp"
#include "Topology.hpp"
#include "TransformationBuffer.hpp"
#include "VertexBuffer.hpp"

namespace gpu_renderer::drawable {
// TODO: Find more approriate names.
Box::Box(Graphics& gfx, std::mt19937& rng,
         std::uniform_real_distribution<float>& angle_dist,
         std::uniform_real_distribution<float>& delta_pos_dist,
         std::uniform_real_distribution<float>& delta_angle_dist,
         std::uniform_real_distribution<float>& radius_dist)
    : r{radius_dist(rng)},
      droll{delta_pos_dist(rng)},
      dpitch{delta_pos_dist(rng)},
      dyaw{delta_pos_dist(rng)},
      dphi{delta_angle_dist(rng)},
      dtheta{delta_angle_dist(rng)},
      dchi{delta_angle_dist(rng)},
      chi{angle_dist(rng)},
      theta{angle_dist(rng)},
      phi{angle_dist(rng)} {
  struct Vertex {
    float x;
    float y;
    float z;
  };

  // TODO: Make it static.
  static constexpr std::array kVertices{
      Vertex{.x = -1.f, .y = -1.f, .z = -1.f},
      Vertex{.x = -1.f, .y = 1.f, .z = -1.f},
      Vertex{.x = 1.f, .y = 1.f, .z = -1.f},
      Vertex{.x = 1.f, .y = -1.f, .z = -1.f},
      Vertex{.x = -1.f, .y = -1.f, .z = 1.f},
      Vertex{.x = -1.f, .y = 1.f, .z = 1.f},
      Vertex{.x = 1.f, .y = 1.f, .z = 1.f},
      Vertex{.x = 1.f, .y = -1.f, .z = 1.f},
  };
  AddBindable(
      std::make_shared<bindable::VertexBuffer>(gfx, gsl::make_span(kVertices)));

  // TODO: Make it static.
  {
    auto vertex_shader =
        std::make_shared<bindable::VertexShader>(gfx, L"VertexShader.cso");
    AddBindable(std::make_shared<bindable::InputLayout>(gfx, *vertex_shader));
    AddBindable(std::move(vertex_shader));
  }
  AddBindable(std::make_shared<bindable::PixelShader>(gfx, L"PixelShader.cso"));

  // TODO: Make it static.
  static constexpr std::array<unsigned short, 36u> kIndicesForTriangle{
      {0u, 1u, 3u, 2u, 3u, 1u, 3u, 2u, 7u, 6u, 7u, 2u, 0u, 4u, 1u, 5u, 1u, 4u,
       6u, 5u, 7u, 4u, 7u, 5u, 1u, 5u, 2u, 6u, 2u, 5u, 4u, 0u, 7u, 3u, 7u, 0u}};
  AddIndexBuffer(std::make_shared<bindable::IndexBuffer>(
      gfx, gsl::span<unsigned short const>{kIndicesForTriangle}));

  // TODO: Make it static.
  static constexpr struct {
    float r;
    float g;
    float b;
    float a;
  } kColors[6u]{{1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f},
                {1.f, 1.f, 0.f}, {0.f, 1.f, 1.f}, {1.f, 0.f, 1.f}};
  AddBindable(std::make_shared<bindable::PixelConstBuffer<decltype(kColors)>>(
      gfx, kColors));
  AddBindable(std::make_shared<
              bindable::Topology<SupportedTypologyType::TRIANGLE_LIST>>());
  AddBindable(std::make_shared<bindable::TransformationBuffer>(gfx, *this));
}

void Box::Update(FrameTimer::DeltaTime dt) {
  constexpr auto wrap_angle = [](float angle) {
    while (angle > 2.f * std::numbers::pi_v<float>)
      angle -= 2.f * std::numbers::pi_v<float>;
    return angle;
  };

  roll = wrap_angle(roll + droll * dt);
  pitch = wrap_angle(pitch + dpitch * dt);
  yaw = wrap_angle(yaw + dyaw * dt);
  theta = wrap_angle(theta + dtheta * dt);
  phi = wrap_angle(phi + dphi * dt);
  chi = wrap_angle(chi + dchi * dt);
}

DirectX::XMMATRIX Box::GetTransformation() const {
  return DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll) *
         DirectX::XMMatrixTranslation(r, 0.f, 0.f) *
         DirectX::XMMatrixRotationRollPitchYaw(theta, phi, chi) *
         DirectX::XMMatrixTranslation(0.f, 0.f, 20.f);
}
}  // namespace gpu_renderer::drawable
