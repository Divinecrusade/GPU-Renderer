#ifndef BOX_HPP
#define BOX_HPP

#include "Drawable.hpp"
#include "ConstBuffer.hpp"

namespace gpu_renderer::drawable {
class Box : public Drawable {
 public:
  Box(Graphics& gfx);

  void Update(FrameTimer::DeltaTime dt, DirectX::XMMATRIX const& projection);

 private:
  static constexpr std::array<unsigned short, 36u> kIndicesForTriangle{
      {0u, 1u, 3u, 2u, 3u, 1u, 3u, 2u, 7u, 6u, 7u, 2u, 0u, 4u, 1u, 5u, 1u, 4u,
       6u, 5u, 7u, 4u, 7u, 5u, 1u, 5u, 2u, 6u, 2u, 5u, 4u, 0u, 7u, 3u, 7u, 0u}};

  float cur_angle_ = 0.f;
  DirectX::XMMATRIX transformation_;
};
}  // namespace gpu_renderer::drawable


#endif  // BOX_HPP
