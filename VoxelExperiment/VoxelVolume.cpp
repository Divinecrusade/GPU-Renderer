#include "VoxelVolume.hpp"

#include <stdexcept>

#include "../IndexBuffer.hpp"
#include "../InputLayout.hpp"
#include "../Shader.hpp"
#include "../Topology.hpp"
#include "../VertexBuffer.hpp"
#include "StructuredBuffer.hpp"

namespace gpu_renderer::drawable {
VoxelVolume::VoxelVolume(Graphics& gfx, voxel::IVoxelGrid const& grid, std::uint32_t viewport_width,
                         std::uint32_t viewport_height)
    : params_{.camera_position = {0.f, 0.f, 0.f},
              .resolution = grid.Resolution(),
              .forward = {0.f, 0.f, 1.f},
              .depth = voxel::Log2Ceil(grid.Resolution()),
              .right = {1.f, 0.f, 0.f},
              .mode = static_cast<std::uint32_t>(grid.GetRepresentation()),
              .up = {0.f, 1.f, 0.f},
              .show_steps = 0u,
              .width = viewport_width,
              .height = viewport_height,
              .blocks_per_side = grid.Resolution() / voxel::BlockGrid::kBlockSide,
              .directory_size = 0u,
              .traversal = static_cast<std::uint32_t>(voxel::Traversal::kStack),
              .padding = {0u, 0u, 0u}} {
  if (voxel::IsDense(grid.GetRepresentation()) && grid.Resolution() > kMaxDenseResolution) {
    throw std::invalid_argument{"Dense grid is limited to N <= 2048 on GPU"};
  }
  params_.directory_size = params_.blocks_per_side * params_.blocks_per_side * params_.blocks_per_side;

  if (!IsCommonInitialized()) {
    struct Vertex {
      float x;
      float y;
      float z;
    };

    // Полноэкранный прямоугольник в пространстве отсечения; z = 0.5 проходит тест глубины.
    static constexpr std::array kVertices{
        Vertex{.x = -1.f, .y = -1.f, .z = 0.5f},
        Vertex{.x = -1.f, .y = 1.f, .z = 0.5f},
        Vertex{.x = 1.f, .y = 1.f, .z = 0.5f},
        Vertex{.x = 1.f, .y = -1.f, .z = 0.5f},
    };
    AddCommonBindable(std::make_shared<bindable::VertexBuffer>(gfx, gsl::make_span(kVertices)));

    {
      auto vertex_shader = std::make_shared<bindable::VertexShader>(gfx, L"VoxelTraceVS.cso");
      AddCommonBindable(std::make_shared<bindable::InputLayout>(gfx, *vertex_shader));
      AddCommonBindable(std::move(vertex_shader));
    }
    AddCommonBindable(std::make_shared<bindable::PixelShader>(gfx, L"VoxelTracePS.cso"));

    // Обход по часовой стрелке (лицевая сторона по умолчанию в D3D11).
    static constexpr std::array<unsigned short, 6u> kIndices{{0u, 1u, 2u, 0u, 2u, 3u}};
    AddCommonBindable(std::make_shared<bindable::IndexBuffer>(gfx, gsl::span<unsigned short const>{kIndices}));
    AddCommonBindable(std::make_shared<bindable::Topology<SupportedTypologyType::TRIANGLE_LIST>>());
  }
  UseCommonIndexBuffer();

  if (grid.GpuElementWords() == 4u) {
    AddBindable(std::make_shared<bindable::PixelStructuredBuffer<1u>>(gfx, grid.GpuData(), 4u));
  } else {
    AddBindable(std::make_shared<bindable::PixelStructuredBuffer<0u>>(gfx, grid.GpuData()));
  }
  params_buffer_ = std::make_shared<bindable::PixelConstBuffer<TraceParams>>(gfx, params_);
  AddBindable(std::shared_ptr<Bindable>{params_buffer_});
}

void VoxelVolume::SetView(Graphics& gfx, voxel::CameraBasis const& basis, bool show_steps) {
  params_.camera_position = basis.position;
  params_.forward = basis.forward;
  params_.right = basis.right;
  params_.up = basis.up;
  params_.show_steps = show_steps ? 1u : 0u;
  params_buffer_->Update(gfx, params_);
}

void VoxelVolume::SetTraversal(Graphics& gfx, voxel::Traversal traversal) {
  params_.traversal = static_cast<std::uint32_t>(traversal);
  params_buffer_->Update(gfx, params_);
}

void VoxelVolume::Update([[maybe_unused]] FrameTimer::DeltaTime dt) {}

DirectX::XMMATRIX VoxelVolume::GetTransformation() const { return DirectX::XMMatrixIdentity(); }
}  // namespace gpu_renderer::drawable
