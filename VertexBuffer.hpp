#ifndef VERTEX_BUFFER_HPP
#define VERTEX_BUFFER_HPP

#include "Graphics.hpp"

namespace gpu_renderer::bindable {
class VertexBuffer : public Graphics::Bindable {
 public:
  template <typename T>
  VertexBuffer(Graphics& gfx, gsl::span<T const> vertices)
      : Bindable{gfx}, stride_{sizeof(T)} {
    constexpr UINT kNoCpuAccessToBuffer = 0u;
    constexpr UINT kNoMisc = 0u;
    D3D11_BUFFER_DESC const vertex_buffer_desc{
        .ByteWidth = gsl::narrow_cast<UINT>(vertices.size() * sizeof(T)),
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = kNoCpuAccessToBuffer,
        .MiscFlags = kNoMisc};
    D3D11_SUBRESOURCE_DATA const vertex_buffer_subres{.pSysMem =
                                                          vertices.data()};

    GetDebugger().StartTraceInDebugMode();
    if (HRESULT const operation_status = GetDevice().CreateBuffer(
            &vertex_buffer_desc, &vertex_buffer_subres, &vertex_buffer_);
        FAILED(operation_status)) {
      throw GetDebugger().CreateDirectXError(
          operation_status, "Vertex buffer was not created",
          "Buffer was not created", __FILEW__, __LINE__);
    }
  }

  void Activate() override {
    constexpr UINT offset = 0u;
    GetDeviceContext().IASetVertexBuffers(0u, 1u, vertex_buffer_.GetAddressOf(),
                                          &stride_, &offset);
  }

 private:
  UINT stride_;
  Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer_{};
};
}  // namespace gpu_renderer::bindable

#endif  // !VERTEX_BUFFER_HPP
