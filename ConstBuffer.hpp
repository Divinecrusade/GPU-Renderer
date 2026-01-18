#ifndef CONST_BUFFER_HPP
#define CONST_BUFFER_HPP

#include "Graphics.hpp"
#include "SupportedShaderType.hpp"

namespace gpu_renderer::bindable {
template <SupportedShaderType T>
class ConstBuffer : public Graphics::Bindable {
 public:
  template <typename Buffer>
  ConstBuffer(Graphics& gfx, Buffer const& buffer_content) : Bindable{gfx} {
    constexpr UINT kNoMisc = 0u;
    constexpr D3D11_BUFFER_DESC buffer_desc{
        .ByteWidth = sizeof(Buffer),
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = kNoMisc};
    D3D11_SUBRESOURCE_DATA const buffer_subres{.pSysMem = &buffer_content};
    
    GetDebugger().StartTraceInDebugMode();
    if (HRESULT const operation_status = GetDevice().CreateBuffer(
            &buffer_desc, &buffer_subres, &const_buffer_);
        FAILED(operation_status)) {
      throw GetDebugger().CreateDirectXError(
          operation_status, "Const buffer was not created",
          "Graphical buffer was not created", 
          __FILEW__, __LINE__);
    }
  }

  void Activate() override {
    GetDebugger().StartTraceInDebugMode();
    if constexpr (T == SupportedShaderType::kVertex) {
      GetDeviceContext().VSSetConstantBuffers(0u, 1u, const_buffer_.GetAddressOf());
    } else if constexpr (T == SupportedShaderType::kPixel) {
      GetDeviceContext().PSSetConstantBuffers(0u, 1u, const_buffer_.GetAddressOf());
    }
  }

 private:
  Microsoft::WRL::ComPtr<ID3D11Buffer> const_buffer_{};
};
}  // namespace gpu_renderer::bindable

#endif  // !CONST_BUFFER_HPP
