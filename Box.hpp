#ifndef BOX_HPP
#define BOX_HPP

#include "Drawable.hpp"
#include "OptimisedStlHeader.hpp"

namespace gpu_renderer::drawable {
class Box : public Drawable {
 public:
  Box(Graphics& gfx, std::mt19937& rng, std::uniform_real_distribution<float>& angle_dist,
  std::uniform_real_distribution<float>& delta_pos_dist,
  std::uniform_real_distribution<float>& delta_angle_dist,
  std::uniform_real_distribution<float>& radius_dist);

  void Update(FrameTimer::DeltaTime dt) override;
  DirectX::XMMATRIX GetTransformation() const override;

 private:
  // positional
  float r;
  float roll = 0.0f;
  float pitch = 0.0f;
  float yaw = 0.0f;
  float theta;
  float phi;
  float chi;
  // speed (delta/s)
  float droll;
  float dpitch;
  float dyaw;
  float dtheta;
  float dphi;
  float dchi;
};
}  // namespace gpu_renderer

#endif  // !BOX_HPP
