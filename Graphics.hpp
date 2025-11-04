#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include "OptimisedWindowsHeader.hpp"
#include "OptimisedDirect3dHeader.hpp"
#include "OptimisedStlHeader.hpp"

#ifdef _DEBUG
#include "DXDebugInfoManager.hpp"
#endif  // _DEBUG
#include "DirectXError.hpp"
#include "DeviceRemovedError.hpp"
#include "DebugLayerMessage.hpp"

namespace gpu_renderer {
class Graphics final {
 public:
  class Color final {
   private:
    static constexpr std::size_t kNumberOfColorComponents = 4u;
    static constexpr std::size_t kIndexComponentR = 0u;
    static constexpr std::size_t kIndexComponentG = 1u;
    static constexpr std::size_t kIndexComponentB = 2u;

   public:
    Color(float r, float g, float b) noexcept;
    
    FLOAT const* operator&() const noexcept;

    [[nodiscard]] float GetR() const noexcept;
    [[nodiscard]] float GetG() const noexcept;
    [[nodiscard]] float GetB() const noexcept;
    
    void SetR(float r) noexcept;
    void SetG(float g) noexcept;
    void SetB(float b) noexcept;

   private:
    std::array<float, kNumberOfColorComponents> encoded_;
  };

 public:
  Graphics() = delete;
  Graphics(HWND hwnd);
  Graphics(Graphics const&) = delete;
  Graphics(Graphics&&) = delete;

  Graphics& operator=(Graphics const&) = delete;
  Graphics& operator=(Graphics&&) = delete;

  ~Graphics() = default;

  void EndFrame();

  void ClearBuffer(Color const& c);

  void DrawTestTriangle(float angle = 0.f) {
#ifdef _DEBUG
    struct ColorVector2D {
      float x = 0.f;
      float y = 0.f;
      unsigned char r = 0u;
      unsigned char g = 0u;
      unsigned char b = 0u;
    };

    struct ConstBuffer {
      struct {
        float matrix[4][4];
      } transformation;
    };
    ConstBuffer const cb = {
      {
        3.f / 4.f * std::cos(angle), std::sin(angle), 0.f, 0.f,
        3.f / 4.f * -std::sin(angle), std::cos(angle), 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
      }
    };

    std::array<ColorVector2D, 9u> vertices{
        ColorVector2D{.x = -0.25f, .y = -0.5f, .r = 255u},
        ColorVector2D{.x = -0.5f, .y = 0.f, .r = 255u},
        ColorVector2D{.x = 0.f, .y = 0.5f, .b = 255u},
        ColorVector2D{.x = 0.5f, .y = 0.f, .g = 255u},
        ColorVector2D{.x = 0.25f, .y = -0.5f, .g = 255u},
    }; 
    std::array<unsigned short, 9u> indices{
        0u, 1u, 2u, 2u, 3u, 4u, 0u, 2u, 4u,
    };

    constexpr UINT kNoCpuAccessToBuffer = 0u;
    constexpr UINT kNoMisc = 0u;

    D3D11_BUFFER_DESC const vertex_buffer_conf{
      .ByteWidth = vertices.size() * sizeof(ColorVector2D),
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_VERTEX_BUFFER,
      .CPUAccessFlags = kNoCpuAccessToBuffer,
      .MiscFlags = kNoMisc
    };
    D3D11_SUBRESOURCE_DATA const vertex_buffer{.pSysMem = vertices.data()};
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffers{};
    StartTraceInDebugMode();
    if (HRESULT const operation_status = 
            device_->CreateBuffer(&vertex_buffer_conf, &vertex_buffer, &vertex_buffers);
        FAILED(operation_status)) {
        throw exception::DirectXError::Create(operation_status,
                                              "Buffer was not created",
                                              "Buffer was not created",
                                              __FILEW__, __LINE__);
    }
    constexpr UINT stride = sizeof(ColorVector2D);
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
    device_context_->IASetIndexBuffer(indices_buffer_ptr.Get(), DXGI_FORMAT_R16_UINT, 0u);

    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout{};
    constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2u> input_element2d_descriptor{
      D3D11_INPUT_ELEMENT_DESC{
        .SemanticName = "Position",
        .SemanticIndex = 0u,
        .Format = DXGI_FORMAT_R32G32_FLOAT,
        .InputSlot = 0u,
        .AlignedByteOffset = 0u,
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0u},
      D3D11_INPUT_ELEMENT_DESC{
          .SemanticName = "Color",
          .SemanticIndex = 0u,
          .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
          .InputSlot = 0u,
          .AlignedByteOffset = sizeof(ColorVector2D::x) + sizeof(ColorVector2D::y),
          .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0u},
      };
    StartTraceInDebugMode();
    if (HRESULT const operation_status = device_->CreateInputLayout(
            input_element2d_descriptor.data(), 2u, shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(), &input_layout);
        FAILED(operation_status)) {
      throw CreateDirectXError(
          operation_status, "Failed to create input layout",
          "Resource for Graphics was not allocated", __FILEW__, __LINE__);
    }

    device_context_->IASetInputLayout(input_layout.Get());
    device_context_->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D11_BUFFER_DESC const const_buffer_conf{
        .ByteWidth = sizeof(cb),
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = kNoMisc};
    D3D11_SUBRESOURCE_DATA const const_buffer{.pSysMem = &cb};
    Microsoft::WRL::ComPtr<ID3D11Buffer> const_buffers{};
    StartTraceInDebugMode();
    if (HRESULT const operation_status = device_->CreateBuffer(
            &const_buffer_conf, &const_buffer, &const_buffers);
        FAILED(operation_status)) {
      throw exception::DirectXError::Create(
          operation_status, "Buffer was not created", "Buffer was not created",
          __FILEW__, __LINE__);
    }
    StartTraceInDebugMode();
    device_context_->VSSetConstantBuffers(0u, 1u, const_buffers.GetAddressOf());

    device_context_->DrawIndexed(indices.size(), 0u, 0u);
    if (auto expected_trace = debug_info_.GetTraceLog(); 
        expected_trace && !expected_trace->empty()) {
      throw exception::DebugLayerMessage{__FILEW__, __LINE__, 
                                         "Vertex buffer setting failed",
                                         std::move(expected_trace.value())};
    }
#endif  // _DEBUG
  }

 private:
  void StartTraceInDebugMode() {
#ifdef _DEBUG
    debug_info_.StartTrace();
#endif  // _DEBUG
  }
  
  exception::DirectXError CreateDirectXError(HRESULT operation_status,
                                             [[maybe_unused]] char const* message_debug,
                                             [[maybe_unused]] char const* message_release,
                                             [[maybe_unused]] wchar_t const* file,
                                             [[maybe_unused]] int line) {
#ifdef _DEBUG
    return {file, line, message_debug, operation_status, debug_info_};
#else
    return {message_release, operation_status};
#endif  // _DEBUG
  }

  exception::DeviceRemovedError CreateDeviceRemovedError(
      ID3D11Device& removed_device, 
      [[maybe_unused]] char const* message_debug,
      [[maybe_unused]] char const* message_release,
      [[maybe_unused]] wchar_t const* file, 
      [[maybe_unused]] int line) {
#ifdef _DEBUG
    return {file, line, message_debug, removed_device, debug_info_};
#else
    return {message_release, removed_device};
#endif  // _DEBUG
  }

 private:
  Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_{};
  Microsoft::WRL::ComPtr<ID3D11Device> device_{};
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context_{};
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_{};
  
  Microsoft::WRL::ComPtr<ID3DBlob> shader_blob{};
#ifdef _DEBUG
  debug::DXDebugInfoManager debug_info_{};
#endif  // _DEBUG
};
}  // namespace gpu_renderer

#endif  // !GRAPHICS_HPP
