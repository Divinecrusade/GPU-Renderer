#ifndef INDEX_BUFFER
#define INDEX_BUFFER

#include "Graphics.hpp"

namespace gpu_renderer::bindable {
class IndexBuffer : public Graphics::Bindable {
  static constexpr UINT kNoMisc = 0u;
  static constexpr UINT kNoCpuAccessToBuffer = 0u;

 public:
  template <std::unsigned_integral T>
  IndexBuffer(gsl::span<T const> indices)
      : Bindable{},
        indices_buffer_conf_{
            .ByteWidth = gsl::narrow<UINT>(indices.size() * sizeof(T)),
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
            .CPUAccessFlags = kNoCpuAccessToBuffer,
            .MiscFlags = kNoMisc},
        indices_buffer_subres_{.pSysMem = indices.data()} {}

  void Bind(Graphics& gfx) override {
    GetDebugger(gfx).StartTraceInDebugMode();
    if (HRESULT const operation_status = GetDevice(gfx).CreateBuffer(
            &indices_buffer_conf_, &indices_buffer_subres_, &indices_buffer_);
        FAILED(operation_status)) {
      throw GetDebugger(gfx).CreateDirectXError(
          operation_status, "Indices buffer was not created",
          "Buffer was not created", __FILEW__, __LINE__);
    }
    GetDebugger(gfx).StartTraceInDebugMode();
    GetDeviceContext(gfx).IASetIndexBuffer(indices_buffer_.Get(),
                                           DXGI_FORMAT_R16_UINT, 0u);
  }

 private:
  D3D11_BUFFER_DESC indices_buffer_conf_;
  D3D11_SUBRESOURCE_DATA indices_buffer_subres_;
  Microsoft::WRL::ComPtr<ID3D11Buffer> indices_buffer_{};
};
}  // namespace gpu_renderer::bindable

#endif  // !INDEX_BUFFER
