#include "FrameCapture.hpp"

#include <cstdio>
#include <span>
#include <stdexcept>
#include <string>

#include "ImageFile.hpp"

namespace gpu_renderer::voxel {
namespace {
void ThrowIfFailed(HRESULT status, char const* what) {
  if (FAILED(status)) {
    throw std::runtime_error{std::string{"FrameCapture: "} + what + " failed (HRESULT 0x" +
                             [status] {
                               char text[9];
                               std::snprintf(text, sizeof(text), "%08lX", static_cast<unsigned long>(status));
                               return std::string{text};
                             }() +
                             ")"};
  }
}
}  // namespace

void FrameCapture::SaveBackBuffer(Graphics& gfx, std::filesystem::path const& path) {
  using Microsoft::WRL::ComPtr;

  ComPtr<ID3D11Texture2D> back_buffer;
  ThrowIfFailed(gfx.swap_chain_->GetBuffer(0u, __uuidof(ID3D11Texture2D), &back_buffer), "GetBuffer");

  D3D11_TEXTURE2D_DESC desc{};
  back_buffer->GetDesc(&desc);
  PixelOrder order = PixelOrder::kRgba;
  if (desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM || desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) {
    order = PixelOrder::kBgra;
  } else if (desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM && desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
    throw std::runtime_error{"FrameCapture: unsupported back buffer format"};
  }
  if (desc.SampleDesc.Count != 1u) {
    throw std::runtime_error{"FrameCapture: multisampled back buffer is not supported"};
  }

  desc.Usage = D3D11_USAGE_STAGING;
  desc.BindFlags = 0u;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.MiscFlags = 0u;
  ComPtr<ID3D11Texture2D> staging;
  ThrowIfFailed(gfx.device_->CreateTexture2D(&desc, nullptr, &staging), "CreateTexture2D (staging)");

  gfx.device_context_->CopyResource(staging.Get(), back_buffer.Get());
  D3D11_MAPPED_SUBRESOURCE mapped{};
  ThrowIfFailed(gfx.device_context_->Map(staging.Get(), 0u, D3D11_MAP_READ, 0u, &mapped), "Map");
  try {
    std::size_t const size = static_cast<std::size_t>(mapped.RowPitch) * desc.Height;
    WriteBmp24(path, desc.Width, desc.Height,
               std::span<std::uint8_t const>{static_cast<std::uint8_t const*>(mapped.pData), size}, mapped.RowPitch,
               order);
  } catch (...) {
    gfx.device_context_->Unmap(staging.Get(), 0u);
    throw;
  }
  gfx.device_context_->Unmap(staging.Get(), 0u);
}
}  // namespace gpu_renderer::voxel
