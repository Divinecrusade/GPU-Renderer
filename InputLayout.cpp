#include "InputLayout.hpp"

#include "OptimisedGslheader.hpp"
#include "OptimisedStlHeader.hpp"

namespace gpu_renderer::bindable {
namespace {
constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 1u> kInputElementDescriptor{
    D3D11_INPUT_ELEMENT_DESC{.SemanticName = "Position",
                             .SemanticIndex = 0u,
                             .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                             .InputSlot = 0u,
                             .AlignedByteOffset = 0u,
                             .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                             .InstanceDataStepRate = 0u},
};
}

InputLayout::InputLayout(VertexShader const& vertex_shader)
    : Bindable{}, vertex_shader_{vertex_shader} {
}

void InputLayout::Bind(Graphics& gfx) {
  GetDebugger(gfx).StartTraceInDebugMode();
  if (HRESULT const operation_status = GetDevice(gfx).CreateInputLayout(
          kInputElementDescriptor.data(),
          gsl::narrow_cast<UINT>(kInputElementDescriptor.size()),
          vertex_shader_.GetByteCode().GetBufferPointer(),
          vertex_shader_.GetByteCode().GetBufferSize(), &input_layout_);
      FAILED(operation_status)) {
    throw GetDebugger(gfx).CreateDirectXError(
        operation_status, "Failed to create input layout",
        "Resource for Graphics was not allocated", __FILEW__, __LINE__);
  }
  GetDeviceContext(gfx).IASetInputLayout(input_layout_.Get());
}
}  // namespace gpu_renderer::bindable
