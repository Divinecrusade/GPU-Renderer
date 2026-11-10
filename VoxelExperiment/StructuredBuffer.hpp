#ifndef STRUCTURED_BUFFER_HPP
#define STRUCTURED_BUFFER_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>

#include "../Bindable.hpp"
#include "../OptimisedGslheader.hpp"
#include "../SupportedShaderType.hpp"

namespace gpu_renderer::bindable {
// Неизменяемый буфер, доступный шейдеру как StructuredBuffer<uint> (element_words = 1)
// или StructuredBuffer<uint4> (element_words = 4) в регистре t<Slot>.
// Создание может завершиться ошибкой, если буфер превышает предел размера ресурса D3D11
// (порядка min(max(128 МБ, 1/4 видеопамяти), 2 ГБ)) — это тоже результат эксперимента.
template <SupportedShaderType T, UINT Slot = 0u>
class StructuredBuffer : public Bindable {
  static constexpr UINT kNoCpuAccessToBuffer = 0u;

 public:
  StructuredBuffer(Graphics& gfx, std::span<std::uint32_t const> words, std::uint32_t element_words = 1u)
      : elements_count_{gsl::narrow<UINT>(std::max<std::size_t>(words.size() / std::max(element_words, 1u), 1u))} {
    if (element_words != 1u && element_words != 4u) {
      throw std::invalid_argument{"Structured buffer element must be uint or uint4"};
    }
    if (words.size() % element_words != 0u) {
      throw std::invalid_argument{"Structured buffer data is not a whole number of elements"};
    }
    static constexpr std::array<std::uint32_t, 4u> kPlaceholder{};  // пустой буфер D3D11 создать нельзя
    UINT const stride = element_words * static_cast<UINT>(sizeof(std::uint32_t));
    D3D11_BUFFER_DESC const buffer_desc{
        .ByteWidth = gsl::narrow<UINT>(static_cast<std::size_t>(elements_count_) * stride),
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = kNoCpuAccessToBuffer,
        .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
        .StructureByteStride = stride};
    D3D11_SUBRESOURCE_DATA const buffer_subres{.pSysMem = words.empty() ? kPlaceholder.data() : words.data()};

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
