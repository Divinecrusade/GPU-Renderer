#include "Application.hpp"
#include "FrameTimer.hpp"

namespace gpu_renderer {
Application::Application(HINSTANCE hInstance, int nCmdShow)
    : window_class_{hInstance, window::Canvas::GetlpfnWndProc()},
      window_{window_class_, kName,   kLeftTopCornerPosX, kLeftTopCornerPosY,
              kWidth,        kHeight, hInstance},
      triangle_indices_buffer_{window_.gfx, std::span<unsigned short const>{kIndicesForTriangle}},
      cube_vertices_buffer_{window_.gfx, std::span<Vector3D const>{kCubeVertices}} {
  window_.Show(nCmdShow);
}

window::ExitCode Application::Run() {
  std::optional<window::ExitCode> exit_code = Process();

  vertex_shader_.Activate();
  pixel_shader_.Activate();
  input_layout_.Activate();
  triangle_indices_buffer_.Activate();
  cube_vertices_buffer_.Activate();

  for (FrameTimer ft{}; !exit_code; exit_code = Process()) {
    Update(ft.Mark());
    Render();
  }

  return *exit_code;
}

std::optional<window::ExitCode> Application::Process() {
  constexpr bool kLockInQueue = false;
  return window_.ProcessMessagesFromQueue<kLockInQueue>();
}

void Application::Update(FrameTimer::DeltaTime dt) {
  cur_angle_ += dt * std::numbers::pi_v<float> / 4.f;
  while (cur_angle_ > 2.f * std::numbers::pi_v<float>)
    cur_angle_ -= 2.f * std::numbers::pi_v<float>;

  for (auto event = window_.GetMouse().GetOldestEvent(); event.has_value();
       event = window_.GetMouse().GetOldestEvent()) {
    if (event->second == input::Mouse::EventType::kMove) {
      y_ = event->first.y * (-2.f / kHeight) + 1.f;
    }
  }

  cube_transformation_ = DirectX::XMMatrixTranspose(
      DirectX::XMMatrixRotationZ(cur_angle_) *
      DirectX::XMMatrixRotationX(cur_angle_) *
      DirectX::XMMatrixTranslation(0.f, 0.f, y_ * 1.5f + 3.f) *
      DirectX::XMMatrixPerspectiveLH(1.f, 3.f / 4.f, 0.2f, 10.f));

  vertex_const_buffer_ = std::make_unique<bindable::VertexConstBuffer>(window_.gfx, cube_transformation_);
  vertex_const_buffer_->Activate();
  pixel_const_buffer_ = std::make_unique<bindable::PixelConstBuffer>(window_.gfx, cube_colors_);
  pixel_const_buffer_->Activate();
}

void Application::Render() { 
  window_.gfx.ClearBuffer({0.f, 0.f, 0.f});
  window_.gfx.DrawIndexed(kIndicesForTriangle.size());
  window_.gfx.EndFrame(); 
}
}  // namespace gpu_renderer
