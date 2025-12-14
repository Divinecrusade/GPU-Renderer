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

void Graphics::DrawTestTriangle(float z, float angle) {
#ifdef _DEBUG
  struct Vector3D {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
  };

  struct VertexConstBuffer {
    DirectX::XMMATRIX transformation;
  };
  VertexConstBuffer const vcb = {DirectX::XMMatrixTranspose(
      DirectX::XMMatrixRotationZ(angle) * DirectX::XMMatrixRotationX(angle) *
      DirectX::XMMatrixTranslation(0.f, 0.f, z) *
      DirectX::XMMatrixPerspectiveLH(1.f, 3.f / 4.f, 0.2f, 10.f))};

  struct PixelConstBuffer {
    struct {
      float r;
      float g;
      float b;
      float a;
    } colors[6u];
  };
  PixelConstBuffer const pcb{{{1.f, 0.f, 0.f},
                              {0.f, 1.f, 0.f},
                              {0.f, 0.f, 1.f},
                              {1.f, 1.f, 0.f},
                              {0.f, 1.f, 1.f},
                              {1.f, 0.f, 1.f}}};

  std::array<Vector3D, 8u> vertices{
      Vector3D{.x = -1.f, .y = -1.f, .z = -1.f},
      Vector3D{.x = -1.f, .y = 1.f, .z = -1.f},
      Vector3D{.x = 1.f, .y = 1.f, .z = -1.f},
      Vector3D{.x = 1.f, .y = -1.f, .z = -1.f},
      Vector3D{.x = -1.f, .y = -1.f, .z = 1.f},
      Vector3D{.x = -1.f, .y = 1.f, .z = 1.f},
      Vector3D{.x = 1.f, .y = 1.f, .z = 1.f},
      Vector3D{.x = 1.f, .y = -1.f, .z = 1.f},
  };
  std::array<unsigned short, 36u> indices{
      0u, 1u, 3u, 2u, 3u, 1u, 3u, 2u, 7u, 6u, 7u, 2u, 0u, 4u, 1u, 5u, 1u, 4u,
      6u, 5u, 7u, 4u, 7u, 5u, 1u, 5u, 2u, 6u, 2u, 5u, 4u, 0u, 7u, 3u, 7u, 0u};

  constexpr UINT kNoCpuAccessToBuffer = 0u;
  constexpr UINT kNoMisc = 0u;

  D3D11_BUFFER_DESC const vertex_buffer_conf{
      .ByteWidth = vertices.size() * sizeof(Vector3D),
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_VERTEX_BUFFER,
      .CPUAccessFlags = kNoCpuAccessToBuffer,
      .MiscFlags = kNoMisc};
  D3D11_SUBRESOURCE_DATA const vertex_buffer{.pSysMem = vertices.data()};
  Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffers{};
  StartTraceInDebugMode();
  if (HRESULT const operation_status = device_->CreateBuffer(
          &vertex_buffer_conf, &vertex_buffer, &vertex_buffers);
      FAILED(operation_status)) {
    throw exception::DirectXError::Create(
        operation_status, "Buffer was not created", "Buffer was not created",
        __FILEW__, __LINE__);
  }
  constexpr UINT stride = sizeof(Vector3D);
  constexpr UINT offset = 0u;
  StartTraceInDebugMode();
  device_context_->IASetVertexBuffers(0u, 1u, vertex_buffers.GetAddressOf(),
                                      &stride, &offset);

  D3D11_BUFFER_DESC const indices_buffer_conf{
      .ByteWidth = indices.size() * sizeof(unsigned short),
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_INDEX_BUFFER,
      .CPUAccessFlags = kNoCpuAccessToBuffer,
      .MiscFlags = kNoMisc};
  D3D11_SUBRESOURCE_DATA const indices_buffer{.pSysMem = indices.data()};
  Microsoft::WRL::ComPtr<ID3D11Buffer> indices_buffer_ptr{};
  StartTraceInDebugMode();
  if (HRESULT const operation_status = device_->CreateBuffer(
          &indices_buffer_conf, &indices_buffer, &indices_buffer_ptr);
      FAILED(operation_status)) {
    throw exception::DirectXError::Create(
        operation_status, "Buffer was not created", "Buffer was not created",
        __FILEW__, __LINE__);
  }
  StartTraceInDebugMode();
  device_context_->IASetIndexBuffer(indices_buffer_ptr.Get(),
                                    DXGI_FORMAT_R16_UINT, 0u);

  device_context_->IASetPrimitiveTopology(
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  D3D11_BUFFER_DESC const const_vertex_buffer_conf{
      .ByteWidth = sizeof(vcb),
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = kNoMisc};
  D3D11_SUBRESOURCE_DATA const_buffer{.pSysMem = &vcb};
  Microsoft::WRL::ComPtr<ID3D11Buffer> const_buffers{};
  StartTraceInDebugMode();
  if (HRESULT const operation_status = device_->CreateBuffer(
          &const_vertex_buffer_conf, &const_buffer, &const_buffers);
      FAILED(operation_status)) {
    throw exception::DirectXError::Create(
        operation_status, "Buffer was not created", "Buffer was not created",
        __FILEW__, __LINE__);
  }
  StartTraceInDebugMode();
  device_context_->VSSetConstantBuffers(0u, 1u, const_buffers.GetAddressOf());

  D3D11_BUFFER_DESC const const_pixel_buffer_conf{
      .ByteWidth = sizeof(pcb),
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = kNoMisc};
  StartTraceInDebugMode();
  const_buffer.pSysMem = &pcb;
  if (HRESULT const operation_status = device_->CreateBuffer(
          &const_pixel_buffer_conf, &const_buffer, &const_buffers);
      FAILED(operation_status)) {
    throw exception::DirectXError::Create(
        operation_status, "Buffer was not created", "Buffer was not created",
        __FILEW__, __LINE__);
  }
  StartTraceInDebugMode();
  device_context_->PSSetConstantBuffers(0u, 1u, const_buffers.GetAddressOf());

  device_context_->DrawIndexed(indices.size(), 0u, 0u);
  if (auto expected_trace = debug_info_.GetTraceLog();
      expected_trace && !expected_trace->empty()) {
    throw exception::DebugLayerMessage{__FILEW__, __LINE__,
                                       "Vertex buffer setting failed",
                                       std::move(expected_trace.value())};
  }
#endif  // _DEBUG
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
