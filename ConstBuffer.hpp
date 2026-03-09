#ifndef CONST_BUFFER_HPP
#define CONST_BUFFER_HPP

#include "Graphics.hpp"
#include "SupportedShaderType.hpp"

namespace gpu_renderer::bindable {
template <SupportedShaderType T, typename Buffer>
class ConstBuffer : public Graphics::Bindable {
 private:
  static constexpr UINT kNoMisc = 0u;
  static constexpr D3D11_BUFFER_DESC buffer_desc{
      .ByteWidth = sizeof(Buffer),
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = kNoMisc};

 public:
  ConstBuffer(Graphics& gfx) : Bindable{gfx} {
    GetDebugger().StartTraceInDebugMode();
    if (HRESULT const operation_status = GetDevice().CreateBuffer(
            &buffer_desc, nullptr, &const_buffer_);
        FAILED(operation_status)) {
      throw GetDebugger().CreateDirectXError(
          operation_status, "Const buffer was not created",
          "Graphical buffer was not created", __FILEW__, __LINE__);
    }
  }

  ConstBuffer(Graphics& gfx, Buffer const& buffer_content) : Bindable{gfx} {
    D3D11_SUBRESOURCE_DATA const buffer_subres{.pSysMem = &buffer_content};

    GetDebugger().StartTraceInDebugMode();
    if (HRESULT const operation_status = GetDevice().CreateBuffer(
            &buffer_desc, &buffer_subres, &const_buffer_);
        FAILED(operation_status)) {
      throw GetDebugger().CreateDirectXError(
          operation_status, "Const buffer was not created",
          "Graphical buffer was not created", __FILEW__, __LINE__);
    }
  }

  void Update(Buffer const& buffer_content) {
    D3D11_MAPPED_SUBRESOURCE mapped_subres{};

    GetDebugger().StartTraceInDebugMode();
    if (HRESULT const operation_status = GetDeviceContext().Map(const_buffer_.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mapped_subres);
        FAILED(operation_status)) {
      throw GetDebugger().CreateDirectXError(
          operation_status, "DeviceContext Map method failed, const buffer is not updated",
          "Failed to update buffer", __FILEW__, __LINE__);
    }
    std::memcpy(mapped_subres.pData, &buffer_content, sizeof(buffer_content));
    GetDeviceContext().Unmap(const_buffer_.Get(), 0u);
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

template <typename Buffer>
using VertexConstBuffer = ConstBuffer<SupportedShaderType::kVertex, Buffer>;
template <typename Buffer>
using PixelConstBuffer = ConstBuffer<SupportedShaderType::kPixel, Buffer>;
}  // namespace gpu_renderer::bindable

#endif  // !CONST_BUFFER_HPP
