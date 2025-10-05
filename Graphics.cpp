#include "Graphics.hpp"

#include "DirectXError.hpp"
#include "DeviceRemovedError.hpp"

#pragma comment(lib, "d3d11")
#pragma comment(lib, "D3DCompiler")

namespace gpu_renderer {
Graphics::Graphics(HWND hwnd) {
  assert(hwnd != NULL);
  __assume(hwnd != NULL);
  
  constexpr UINT kUseWindowDimension = 0u;
  constexpr UINT kUseDoubleBuffer = 1u;
  constexpr UINT kUseDeafultSwapChainBehaviorOptions = 0u;
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

#pragma warning(push)
#pragma warning(disable : 26462)
  constexpr IDXGIAdapter* kDefaultAdapter = NULL;
  constexpr HMODULE kNoSoftwareRasterizer = NULL;
#ifndef _DEBUG
  constexpr UINT kNoRuntimeLayers = 0u;
#endif  // !_DEBUG
  constexpr D3D_FEATURE_LEVEL* kDefaultFeatureLevel = NULL;
  constexpr UINT kUseDefaultNumberOfFeatures = 0u;
  constexpr D3D_FEATURE_LEVEL* kIgnoreFeatureLevelReturn = NULL;
#pragma warning(pop)

  StartTraceInDebugMode();
  
  if (HRESULT const operation_status = D3D11CreateDeviceAndSwapChain(
          kDefaultAdapter, D3D_DRIVER_TYPE_HARDWARE, kNoSoftwareRasterizer,
#ifdef _DEBUG
          D3D11_CREATE_DEVICE_DEBUG,
#else
          kNoRuntimeLayers,
#endif  // _DEBUG
          kDefaultFeatureLevel, kUseDefaultNumberOfFeatures,
          D3D11_SDK_VERSION, &swap_chain_conf, &swap_chain_, &device_,
          kIgnoreFeatureLevelReturn, &device_context_);
      FAILED(operation_status)) {
    throw CreateDirectXError(operation_status,
                             "Creating device and swap chain failed",
                             "Resource for Graphics was not allocated",
                             __FILEW__, __LINE__);
  }
  assert(((void)"Device must be allocated at this point", device_));
  __assume(device_);
  assert(((void)"Device context must be allocated at this point", 
          device_context_));
  __assume(device_context_);
  assert(((void)"Swap chain must be allocated at this point", 
          swap_chain_));
  __assume(swap_chain_);

  Microsoft::WRL::ComPtr<ID3DBlob> shader_blob{};

  StartTraceInDebugMode();

  if (HRESULT const operation_status = 
          D3DReadFileToBlob(L"VertexShader.cso", &shader_blob);
      FAILED(operation_status)) {
    throw CreateDirectXError(operation_status, 
                             "Failed to read compiled vertex shader into blob",
                             "Error during shader loading",
                             __FILEW__, __LINE__);
  }

  Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader{};

  StartTraceInDebugMode();

  constexpr ID3D11ClassInstance* const* kNoInterfacesForShader = NULL;

  if (HRESULT const operation_status = 
          device_->CreateVertexShader(shader_blob->GetBufferPointer(),
                                      shader_blob->GetBufferSize(),
                                      NULL,
                                      &vertex_shader);
      FAILED(operation_status)) {
    throw CreateDirectXError(operation_status, 
                             "Failed to create vertex shader from blob",
                             "Error during shader loading", 
                             __FILEW__, __LINE__);
  }
  device_context_->VSSetShader(vertex_shader.Get(), 
                               kNoInterfacesForShader,
                               0);

  if (HRESULT const operation_status =
          D3DReadFileToBlob(L"PixelShader.cso", &shader_blob);
      FAILED(operation_status)) {
    throw CreateDirectXError(operation_status, 
                             "Failed to read compiled pixel shader into blob",
                             "Error during shader loading", 
                             __FILEW__, __LINE__);
  }

  Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader{};

  StartTraceInDebugMode();

  if (HRESULT const operation_status = 
          device_->CreatePixelShader(shader_blob->GetBufferPointer(), 
                                      shader_blob->GetBufferSize(), 
                                      NULL,
                                      &pixel_shader);
      FAILED(operation_status)) {
    throw CreateDirectXError(operation_status, 
                             "Failed to create pixel shader from blob",
                             "Error during shader loading", 
                             __FILEW__, __LINE__);
  }
  device_context_->PSSetShader(pixel_shader.Get(), 
                               kNoInterfacesForShader, 
                               0);

  static constexpr UINT kBackBufferId = 0u;
  Microsoft::WRL::ComPtr<ID3D11Resource> back_buffer{};
  
  StartTraceInDebugMode();

#pragma warning(push)
#pragma warning(disable : 26490)
  if (HRESULT const operation_status =
          swap_chain_->GetBuffer(kBackBufferId, __uuidof(ID3D11Resource),
                                 &back_buffer);
#pragma warning(pop)
      FAILED(operation_status)) {
    throw CreateDirectXError(operation_status, 
                             "Backbuffer wasn't created",
                             "Resource for Graphics was not allocated",
                             __FILEW__, __LINE__);
  }
  assert(((void)"Back buffer context must be allocated at this point", 
          back_buffer));
  __assume(back_buffer);

  static constexpr D3D11_RENDER_TARGET_VIEW_DESC const*
      kGiveAccessToAllMipmapLevels = nullptr;

  StartTraceInDebugMode();

  if (HRESULT const operation_status = device_->CreateRenderTargetView(
          back_buffer.Get(), kGiveAccessToAllMipmapLevels, &render_target_);
      FAILED(operation_status)) {
    throw CreateDirectXError(operation_status, 
                             "Target view wasn't created",
                             "Resource for Graphics was not allocated", 
                             __FILEW__, __LINE__);
  }
  assert(((void)"Render target view must be allocated at this point", 
          render_target_));
}

void Graphics::EndFrame() {
  static constexpr UINT kNoSwapChainSync = 0u;
  static constexpr UINT kDefaultSwapChainPresention = 0u;

  StartTraceInDebugMode();

  if (HRESULT const operation_status = 
      swap_chain_->Present(kNoSwapChainSync, kDefaultSwapChainPresention);
      FAILED(operation_status)) {
      if (exception::DeviceRemovedError::IsDeviceRemovedErrorHappened(operation_status)) {
        throw CreateDeviceRemovedError(*device_.Get(), 
                                       "Device removed unexpectly and frame wasn't present",
                                       "Render failed", 
                                       __FILEW__, __LINE__);  
      }
      else {
        throw CreateDirectXError(operation_status,
                                 "Back buffer wasn't present (swapped with front buffer)", 
                                  "Render failed",
                                  __FILEW__, __LINE__);
      }
  }
}

void Graphics::ClearBuffer(Color const& c) {
  device_context_->ClearRenderTargetView(render_target_.Get(), &c);
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
  encoded_[kIndexComponentG] = g;
}

void Graphics::Color::SetB(float b) noexcept {
  assert(((void)"Color component must be normalised (0..1)",
          (0.f <= b && b <= 1.f)));
  __assume((0.f <= b && b <= 1.f));
  encoded_[kIndexComponentB] = b;
}
}  // namespace gpu_renderer
