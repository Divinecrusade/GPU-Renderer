#ifndef INDEX_BUFFER
#define INDEX_BUFFER

#include "Graphics.hpp"

namespace gpu_renderer::bindable {
class IndexBuffer : public Graphics::Bindable {
 public:
  template<std::unsigned_integral T>
  IndexBuffer(Graphics& gfx, gsl::span<T const> indices)
      : Bindable{gfx} {
    constexpr UINT kNoMisc = 0u;
    constexpr UINT kNoCpuAccessToBuffer = 0u;
    D3D11_BUFFER_DESC const indices_buffer_conf{
        .ByteWidth = gsl::narrow<UINT>(indices.size() * sizeof(T)),
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
        .CPUAccessFlags = kNoCpuAccessToBuffer,
        .MiscFlags = kNoMisc};

    D3D11_SUBRESOURCE_DATA const indices_buffer{.pSysMem = indices.data()};
    GetDebugger().StartTraceInDebugMode();
    if (HRESULT const operation_status = GetDevice().CreateBuffer(
            &indices_buffer_conf, &indices_buffer, &indices_buffer_);
        FAILED(operation_status)) {
      throw GetDebugger().CreateDirectXError(operation_status,
                                             "Indices buffer was not created",
                                             "Buffer was not created",
                                             __FILEW__, __LINE__);
    }
  }

  void Activate() override {
    GetDebugger().StartTraceInDebugMode();
    GetDeviceContext().IASetIndexBuffer(indices_buffer_.Get(),
                                        DXGI_FORMAT_R16_UINT, 0u);
  }

 private:
  Microsoft::WRL::ComPtr<ID3D11Buffer> indices_buffer_{};
};
}  // namespace gpu_renderer::bindable

#endif  // !INDEX_BUFFER
