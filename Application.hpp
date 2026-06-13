#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Canvas.hpp"
#include "FrameTimer.hpp"
#include "InputLayout.hpp"
#include "IndexBuffer.hpp"
#include "ConstBuffer.hpp"
#include "VertexBuffer.hpp"
#include "Topology.hpp"

namespace gpu_renderer {
class Application final {
 private:
  static constexpr int kWidth = 640;
  static constexpr int kHeight = 480;
  static constexpr int kLeftTopCornerPosX = 15;
  static constexpr int kLeftTopCornerPosY = 30;
  static constexpr LPCWSTR kName = L"GPU-Renderer";

 public:
  Application(HINSTANCE hInstance, int nCmdShow);

  window::ExitCode Run();

 private:
  std::optional<window::ExitCode> Process() noexcept;
  void Update(FrameTimer::DeltaTime dt);
  void Render();

 private:
  window::CachedDC window_class_;
  window::Canvas wnd_;

  float cur_angle_ = 0.f;
  float y_ = 1.f;

  bindable::VertexShader vertex_shader_;
  bindable::PixelShader pixel_shader_;
  bindable::InputLayout input_layout_;

  static constexpr std::array<unsigned short, 36u> kIndicesForTriangle {
      {0u, 1u, 3u, 2u, 3u, 1u, 3u, 2u, 7u, 6u, 7u, 2u, 0u, 4u, 1u, 5u, 1u,
      4u, 6u, 5u, 7u, 4u, 7u, 5u, 1u, 5u, 2u, 6u, 2u, 5u, 4u, 0u, 7u, 3u,
      7u, 0u}};
  bindable::IndexBuffer triangle_indices_buffer_;

  DirectX::XMMATRIX cube_transformation_{};
  static constexpr struct {
    float r;
    float g;
    float b;
    float a;
  } cube_colors_[6u]{{1.f, 0.f, 0.f},
                     {0.f, 1.f, 0.f},
                     {0.f, 0.f, 1.f},
                     {1.f, 1.f, 0.f},
                     {0.f, 1.f, 1.f},
                     {1.f, 0.f, 1.f}};

  bindable::VertexConstBuffer<DirectX::XMMATRIX> vertex_const_buffer_;
  bindable::PixelConstBuffer<decltype(cube_colors_)> pixel_const_buffer_;

  struct Vector3D {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
  };
  static constexpr std::array<Vector3D, 8u> kCubeVertices{
      Vector3D{.x = -1.f, .y = -1.f, .z = -1.f},
      Vector3D{.x = -1.f, .y = 1.f, .z = -1.f},
      Vector3D{.x = 1.f, .y = 1.f, .z = -1.f},
      Vector3D{.x = 1.f, .y = -1.f, .z = -1.f},
      Vector3D{.x = -1.f, .y = -1.f, .z = 1.f},
      Vector3D{.x = -1.f, .y = 1.f, .z = 1.f},
      Vector3D{.x = 1.f, .y = 1.f, .z = 1.f},
      Vector3D{.x = 1.f, .y = -1.f, .z = 1.f},
  };

  bindable::VertexBuffer cube_vertices_buffer_;
  bindable::Topology<SupportedTypologyType::TRIANGLE_LIST> aTriangleListTopo_;
};
}  // namespace gpu_renderer

#endif  // !APPLICATION_HPP
