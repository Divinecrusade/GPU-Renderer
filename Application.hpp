#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Canvas.hpp"
#include "FrameTimer.hpp"
#include "InputLayout.hpp"
#include "IndexBuffer.hpp"

namespace gpu_renderer {
class Application final {
 private:
  static constexpr int kWidth = 640;
  static constexpr int kHeight = 480;
  static constexpr int kLeftTopCornerPosX = 15;
  static constexpr int kLeftTopCornerPosY = 30;
  static constexpr LPCWSTR kName = L"GPU-Renderer";

 public:
  Application() = delete;
  Application(HINSTANCE hInstance, int nCmdShow);
  Application(Application const&) = delete;
  Application(Application&&) = delete;

  Application& operator=(Application const&) = delete;
  Application& operator=(Application&&) = delete;

  ~Application() = default;

  window::ExitCode Run();
 
 private:
  std::optional<window::ExitCode> Process();
  void Update(FrameTimer::DeltaTime dt);
  void Render();

 private:
  window::CachedDC window_class_;
  window::Canvas window_;

  float cur_angle_ = 0.f;
  float y_ = 1.f;

  bindable::Shader<SupportedShaderType::kVertex> vertex_shader_{L"VertexShader.cso", window_.gfx};
  bindable::Shader<SupportedShaderType::kPixel> pixel_shader_{L"PixelShader.cso", window_.gfx};
  bindable::InputLayout input_layout_{vertex_shader_};

  static constexpr std::array<unsigned short, 36u> kIndicesForTriangle {
      0u, 1u, 3u, 2u, 3u, 1u, 3u, 2u, 7u, 6u, 7u, 2u, 0u, 4u, 1u, 5u, 1u,
      4u, 6u, 5u, 7u, 4u, 7u, 5u, 1u, 5u, 2u, 6u, 2u, 5u, 4u, 0u, 7u, 3u,
      7u, 0u};
  bindable::IndexBuffer triangle_indices_buffer_;
};
}  // namespace gpu_renderer

#endif  // !APPLICATION_HPP
