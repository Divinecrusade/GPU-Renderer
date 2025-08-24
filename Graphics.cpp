#include "Graphics.hpp"

#pragma comment(lib, "d3d11")

namespace gpu_renderer {
Graphics::Graphics(HWND hwnd) {
  assert(((void)"HWND cannot be null", hwnd != NULL));
  __assume(hwnd != NULL);

  static constexpr UINT kUseWindowDimension{0u};
  static constexpr UINT kUseDoubleBuffer{1u};
  static constexpr UINT kUseDeafultSwapChainBehaviorOptions{0u};
  DXGI_SWAP_CHAIN_DESC const swap_chain_conf{
      .BufferDesc = {.Width = kUseWindowDimension,
                     .Height = kUseWindowDimension,
                     .RefreshRate =
                         DXGI_RATIONAL{.Numerator = 0u, .Denominator = 0u},
                     .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                     .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                     .Scaling = DXGI_MODE_SCALING_UNSPECIFIED},
      .SampleDesc = {.Count = 1, .Quality = 0},
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = kUseDoubleBuffer,
      .OutputWindow = hwnd,
      .Windowed = TRUE,
      .SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
      .Flags = kUseDeafultSwapChainBehaviorOptions};

  static constexpr IDXGIAdapter* kDefaultAdapter{nullptr};
  static constexpr HMODULE kNoSoftwareRasterizer{NULL};
  static constexpr UINT kNoRuntimeLayers{0u};
  static constexpr D3D_FEATURE_LEVEL* kDefaultFeatureLevel{nullptr};
  static constexpr UINT kUseDefaultNumberOfFeatures{0u};
  static constexpr D3D_FEATURE_LEVEL* kIgnoreFeatureLevelReturn{nullptr};
  std::ignore = D3D11CreateDeviceAndSwapChain(
      kDefaultAdapter, D3D_DRIVER_TYPE_HARDWARE, kNoSoftwareRasterizer,
      kNoRuntimeLayers, kDefaultFeatureLevel, kUseDefaultNumberOfFeatures,
      D3D11_SDK_VERSION, &swap_chain_conf, &swap_chain_, &device_,
      kIgnoreFeatureLevelReturn, &device_context_);
  assert(device_);
  assert(device_context_);
  assert(swap_chain_);

  ID3D11Resource* back_buffer{nullptr};
  static constexpr UINT kBackBufferId{0u};
#pragma warning(push)
#pragma warning(disable : 26490)
  std::ignore = swap_chain_->GetBuffer(kBackBufferId, __uuidof(ID3D11Resource),
                                       reinterpret_cast<void**>(&back_buffer));
#pragma warning(pop)
  assert(back_buffer);

  static constexpr D3D11_RENDER_TARGET_VIEW_DESC const*
      kGiveAccessToAllMipmapLevels{nullptr};
  std::ignore = device_->CreateRenderTargetView(
      back_buffer, kGiveAccessToAllMipmapLevels, &target_);
  assert(target_);

  std::ignore = back_buffer->Release();
}

Graphics::~Graphics() noexcept {
  assert(target_);
  assert(swap_chain_);
  assert(device_context_);
  assert(device_);
  try {
    std::ignore = target_->Release();
    std::ignore = swap_chain_->Release();
    std::ignore = device_context_->Release();
    std::ignore = device_->Release();
  } catch (...) {
#ifdef LOG_GRAPHICS
    try {
      std::wcerr << L"Unknown exception happened in Graphics destructor\n";
    } catch (...) {
      OutputDebugStringW(
          L"Unknown exception happened in Graphics destructor logging\n");
    }
#endif  // LOG_GRAPHICS
  }
}

void Graphics::EndFrame() {
  static constexpr UINT kNoSwapChainSync{0u};
  static constexpr UINT kDefaultSwapChainPresention{0u};
  std::ignore =
      swap_chain_->Present(kNoSwapChainSync, kDefaultSwapChainPresention);
}

void Graphics::ClearBuffer(Color const& c) {
  device_context_->ClearRenderTargetView(target_, &c);
}

Graphics::Color::Color(float r, float g, float b) noexcept
    : encoded_{{r, g, b, 1.f}} {
  assert(((void)"Color component must be normalised (0..1)",
          (0.f <= r && r <= 1.f)));
  __assume((0.f <= r && r <= 1.f));
  assert(((void)"Color component must be normalised (0..1)",
          (0.f <= g && g <= 1.f)));
  __assume((0.f <= g && g <= 1.f));
  assert(((void)"Color component must be normalised (0..1)",
          (0.f <= b && b <= 1.f)));
  __assume((0.f <= b && b <= 1.f));
}

FLOAT const* Graphics::Color::operator&() const noexcept {
  return encoded_.data();
}

#pragma warning(push)
#pragma warning(disable : 26446)
float Graphics::Color::GetR() const noexcept {
  return encoded_[kIndexComponentR];
}

float Graphics::Color::GetG() const noexcept {
  return encoded_[kIndexComponentG];
}

float Graphics::Color::GetB() const noexcept {
  return encoded_[kIndexComponentB];
}

void Graphics::Color::SetR(float r) noexcept {
  assert(((void)"Color component must be normalised (0..1)",
          (0.f <= r && r <= 1.f)));
  __assume((0.f <= r && r <= 1.f));
  encoded_[kIndexComponentR] = r;
}

void Graphics::Color::SetG(float g) noexcept {
  assert(((void)"Color component must be normalised (0..1)",
          (0.f <= g && g <= 1.f)));
  __assume((0.f <= g && g <= 1.f));
  encoded_[kIndexComponentR] = g;
}

void Graphics::Color::SetB(float b) noexcept {
  assert(((void)"Color component must be normalised (0..1)",
          (0.f <= b && b <= 1.f)));
  __assume((0.f <= b && b <= 1.f));
  encoded_[kIndexComponentR] = b;
}
#pragma warning(pop)
}  // namespace gpu_renderer
