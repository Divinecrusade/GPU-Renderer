#ifndef STRUCTURED_BUFFER_HPP
#define STRUCTURED_BUFFER_HPP

#include <algorithm>
#include <cstdint>
#include <span>

#include "../Bindable.hpp"
#include "../OptimisedGslheader.hpp"
#include "../SupportedShaderType.hpp"

namespace gpu_renderer::bindable {
// Неизменяемый буфер, доступный шейдеру как StructuredBuffer<uint> в регистре t<Slot>.
// Создание может завершиться ошибкой, если буфер превышает предел размера ресурса D3D11
// (порядка min(max(128 МБ, 1/4 видеопамяти), 2 ГБ)) — это тоже результат эксперимента.
template <SupportedShaderType T, UINT Slot = 0u>
class StructuredBuffer : public Bindable {
  static constexpr UINT kNoCpuAccessToBuffer = 0u;

 public:
  StructuredBuffer(Graphics& gfx, std::span<std::uint32_t const> elements)
      : elements_count_{gsl::narrow<UINT>(std::max<std::size_t>(elements.size(), 1u))} {
    static constexpr std::uint32_t kPlaceholder = 0u;  // пустой буфер D3D11 создать нельзя
    D3D11_BUFFER_DESC const buffer_desc{
        .ByteWidth = gsl::narrow<UINT>(static_cast<std::size_t>(elements_count_) * sizeof(std::uint32_t)),
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = kNoCpuAccessToBuffer,
        .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
        .StructureByteStride = sizeof(std::uint32_t)};
    D3D11_SUBRESOURCE_DATA const buffer_subres{.pSysMem = elements.empty() ? &kPlaceholder : elements.data()};

    GetDebugger(gfx).StartTraceInDebugMode();
    if (HRESULT const operation_status = GetDevice(gfx).CreateBuffer(&buffer_desc, &buffer_subres, &buffer_);
        FAILED(operation_status)) {
      throw GetDebugger(gfx).CreateDirectXError(operation_status, "Structured buffer was not created",
                                                "Buffer was not created", __FILEW__, __LINE__);
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC view_desc{};
    view_desc.Format = DXGI_FORMAT_UNKNOWN;
    view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    view_desc.Buffer.FirstElement = 0u;
    view_desc.Buffer.NumElements = elements_count_;

    GetDebugger(gfx).StartTraceInDebugMode();
    if (HRESULT const operation_status = GetDevice(gfx).CreateShaderResourceView(buffer_.Get(), &view_desc, &view_);
        FAILED(operation_status)) {
      throw GetDebugger(gfx).CreateDirectXError(operation_status, "Shader resource view was not created",
                                                "Buffer view was not created", __FILEW__, __LINE__);
    }
  }

  void Bind(Graphics& gfx) override {
    GetDebugger(gfx).StartTraceInDebugMode();
    if constexpr (T == SupportedShaderType::kPixel) {
      GetDeviceContext(gfx).PSSetShaderResources(Slot, 1u, view_.GetAddressOf());
    } else if constexpr (T == SupportedShaderType::kVertex) {
      GetDeviceContext(gfx).VSSetShaderResources(Slot, 1u, view_.GetAddressOf());
    }
  }

  [[nodiscard]] UINT Count() const noexcept { return elements_count_; }

 private:
  UINT elements_count_;
  Microsoft::WRL::ComPtr<ID3D11Buffer> buffer_{};
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_{};
};

template <UINT Slot = 0u>
using PixelStructuredBuffer = StructuredBuffer<SupportedShaderType::kPixel, Slot>;
}  // namespace gpu_renderer::bindable

#endif  // !STRUCTURED_BUFFER_HPP
