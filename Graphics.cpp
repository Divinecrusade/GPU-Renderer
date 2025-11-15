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

  StartTraceInDebugMode();

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
                                     shader_blob->GetBufferSize(), NULL,
                                     &pixel_shader);
      FAILED(operation_status)) {
    throw CreateDirectXError(
        operation_status, "Failed to create pixel shader from blob",
        "Error during shader loading", __FILEW__, __LINE__);
  }
#pragma warning(push)
#pragma warning(disable : 26462)
  constexpr ID3D11ClassInstance* const* kNoInterfacesForShader = NULL;
#pragma warning(pop)
  device_context_->PSSetShader(pixel_shader.Get(), kNoInterfacesForShader, 0);


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

  constexpr D3D11_VIEWPORT kViewPortSettings{
    .TopLeftX = 0.f,
    .TopLeftY = 0.f,
    .Width = 640.f,
    .Height = 480.f,
    .MinDepth = 0.f,
    .MaxDepth = 1.f
  };
  device_context_->RSSetViewports(1u, &kViewPortSettings);

  D3D11_DEPTH_STENCIL_DESC const depth_buf_desc{
    .DepthEnable = TRUE,
    .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
    .DepthFunc = D3D11_COMPARISON_LESS
  };
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_buf{};

  StartTraceInDebugMode();
  device_->CreateDepthStencilState(&depth_buf_desc, &depth_buf);

  StartTraceInDebugMode();
  device_context_->OMSetDepthStencilState(depth_buf.Get(), 1u);
  D3D11_TEXTURE2D_DESC const depth_texture_desc{
    .Width = 640,
    .Height = 480,
    .MipLevels = 1,
    .ArraySize = 1,
    .Format = DXGI_FORMAT_D32_FLOAT,
    .SampleDesc = {.Count = 1},
    .BindFlags = D3D11_BIND_DEPTH_STENCIL
  };
  Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_texture{};

  StartTraceInDebugMode();
  if (HRESULT const operation_status = device_->CreateTexture2D(
          &depth_texture_desc, nullptr, &depth_texture);
      FAILED(operation_status)) {
    throw CreateDirectXError(operation_status, 
                             "Texture for depth buffer wasn't created",
                             "Resource for Graphics was not allocated",
                             __FILEW__, __LINE__);
  }
  

  D3D11_DEPTH_STENCIL_VIEW_DESC const depth_buf_view_desc{
    .Format = DXGI_FORMAT_D32_FLOAT,
    .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
    .Flags = 0u
  };

  StartTraceInDebugMode();
  device_->CreateDepthStencilView(depth_texture.Get(), 
                                  &depth_buf_view_desc, 
                                  &depth_buf_view_);
  StartTraceInDebugMode();
  device_context_->OMSetRenderTargets(1u, render_target_.GetAddressOf(), 
                                      depth_buf_view_.Get());

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
  device_context_->ClearDepthStencilView(depth_buf_view_.Get(), 
                                         D3D11_CLEAR_DEPTH, 1.f, 0u);
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
