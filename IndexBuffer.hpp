#ifndef INDEX_BUFFER
#define INDEX_BUFFER

#include "Bindable.hpp"
#include "OptimisedGslheader.hpp"

namespace gpu_renderer::bindable {
class IndexBuffer : public Bindable {
  static constexpr UINT kNoMisc = 0u;
  static constexpr UINT kNoCpuAccessToBuffer = 0u;

 public:
  template <std::unsigned_integral T>
  IndexBuffer(Graphics& gfx, gsl::span<T const> indices) :
  indices_count_{gsl::narrow<UINT> (indices.size())} {
    D3D11_BUFFER_DESC const indices_buffer_conf{
        .ByteWidth = gsl::narrow<UINT>(indices.size() * sizeof(T)),
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
        .CPUAccessFlags = kNoCpuAccessToBuffer,
        .MiscFlags = kNoMisc};
    D3D11_SUBRESOURCE_DATA const indices_buffer_subres{.pSysMem =
                                                           indices.data()};

    GetDebugger(gfx)
        .StartTraceInDebugMode();
    if (HRESULT const operation_status = GetDevice(gfx).CreateBuffer(
            &indices_buffer_conf, &indices_buffer_subres, &indices_buffer_);
        FAILED(operation_status)) {
      throw GetDebugger(gfx).CreateDirectXError(
          operation_status, "Indices buffer was not created",
          "Buffer was not created", __FILEW__, __LINE__);
    }
  }

  void Bind(Graphics& gfx) override {
    GetDebugger(gfx).StartTraceInDebugMode();
    GetDeviceContext(gfx).IASetIndexBuffer(indices_buffer_.Get(),
                                           DXGI_FORMAT_R16_UINT, 0u);
  }

  UINT Count() const noexcept
  {
    return indices_count_;
  }

 private:
  UINT indices_count_;
  Microsoft::WRL::ComPtr<ID3D11Buffer> indices_buffer_{};
};
}  // namespace gpu_renderer::bindable

#endif  // !INDEX_BUFFER
