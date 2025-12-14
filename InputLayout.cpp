#include "InputLayout.hpp"

#include "OptimisedStlHeader.hpp"

namespace gpu_renderer::bindable {
InputLayout::InputLayout(Shader<SupportedShaderType::kVertex> const& vertex_shader)
    : Bindable{vertex_shader} {
  constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 1u> input_element_descriptor{
      D3D11_INPUT_ELEMENT_DESC{.SemanticName = "Position",
                               .SemanticIndex = 0u,
                               .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                               .InputSlot = 0u,
                               .AlignedByteOffset = 0u,
                               .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                               .InstanceDataStepRate = 0u},
  };
  //StartTraceInDebugMode();
  //if (HRESULT const operation_status = device_->CreateInputLayout(
  //        input_element3d_descriptor.data(), input_element3d_descriptor.size(),
  //        vertex_shader.GetByteCode().GetBufferPointer(),
  //        vertex_shader.GetByteCode().GetBufferSize(), &input_layout);
  //    FAILED(operation_status)) {
  //  throw CreateDirectXError(operation_status, "Failed to create input layout",
  //                           "Resource for Graphics was not allocated",
  //                           __FILEW__, __LINE__);
  //}

  assert(SUCCEEDED(GetDevice().CreateInputLayout(
      input_element_descriptor.data(), input_element_descriptor.size(),
          vertex_shader.GetByteCode().GetBufferPointer(),
          vertex_shader.GetByteCode().GetBufferSize(), &input_layout_)));
}

void InputLayout::Activate() {
  GetDeviceContext().IASetInputLayout(input_layout_.Get());
}
}  // namespace gpu_renderer::bindable
