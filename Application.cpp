#include "Application.hpp"
#include "FrameTimer.hpp"

namespace gpu_renderer {
Application::Application(HINSTANCE hInstance, int nCmdShow)
    : window_class_{hInstance, window::Canvas::GetlpfnWndProc()},
      wnd_{window_class_, kName,   kLeftTopCornerPosX, kLeftTopCornerPosY,
              kWidth,        kHeight, hInstance},
      vertex_shader_{wnd_.gfx, L"VertexShader.cso"},
      pixel_shader_{wnd_.gfx, L"PixelShader.cso"},
      input_layout_{wnd_.gfx, vertex_shader_},
      triangle_indices_buffer_{wnd_.gfx, gsl::span<unsigned short const>{kIndicesForTriangle}},
      vertex_const_buffer_{wnd_.gfx},
      pixel_const_buffer_{wnd_.gfx, cube_colors_},
      cube_vertices_buffer_{wnd_.gfx, gsl::span<Vector3D const>{kCubeVertices}},
      aTriangleListTopo_{} {
  wnd_.Show(nCmdShow);
  wnd_.gfx.SetProjection(DirectX::XMMatrixPerspectiveLH(1.f, 3.f / 4.f, 0.2f, 10.f));
}

window::ExitCode Application::Run() {
  std::optional<window::ExitCode> exit_code = Process();

  vertex_shader_.Bind(wnd_.gfx);
  pixel_shader_.Bind(wnd_.gfx);
  input_layout_.Bind(wnd_.gfx);
  triangle_indices_buffer_.Bind(wnd_.gfx);
  vertex_const_buffer_.Bind(wnd_.gfx);
  pixel_const_buffer_.Bind(wnd_.gfx);
  cube_vertices_buffer_.Bind(wnd_.gfx);
  aTriangleListTopo_.Bind(wnd_.gfx);

  for (FrameTimer ft{}; !exit_code; exit_code = Process()) {
    Update(ft.Mark());
    Render();
  }

  return *exit_code;
}

std::optional<window::ExitCode> Application::Process() noexcept {
  constexpr bool kLockInQueue = false;
  return wnd_.ProcessMessagesFromQueue<kLockInQueue>();
}

void Application::Update(FrameTimer::DeltaTime dt) {
  cur_angle_ += dt * std::numbers::pi_v<float> / 4.f;
  while (cur_angle_ > 2.f * std::numbers::pi_v<float>)
    cur_angle_ -= 2.f * std::numbers::pi_v<float>;

  for (auto event = wnd_.GetMouse().GetOldestEvent(); event.has_value();
       event = wnd_.GetMouse().GetOldestEvent()) {
    if (event->second == input::Mouse::EventType::kMove) {
      y_ = event->first.y * (-2.f / kHeight) + 1.f;
    }
  }

  cube_transformation_ = DirectX::XMMatrixTranspose(
      DirectX::XMMatrixRotationZ(cur_angle_) *
      DirectX::XMMatrixRotationX(cur_angle_) *
      DirectX::XMMatrixTranslation(0.f, 0.f, y_ * 1.5f + 3.f) *
      wnd_.gfx.GetProjection());

  vertex_const_buffer_.Update(wnd_.gfx, cube_transformation_);
}

void Application::Render() {
  wnd_.gfx.ClearBuffer({0.f, 0.f, 0.f});
  wnd_.gfx.DrawIndexed(gsl::narrow_cast<UINT>(kIndicesForTriangle.size()));
  wnd_.gfx.EndFrame();
}
}  // namespace gpu_renderer
