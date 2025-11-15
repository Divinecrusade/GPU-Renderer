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

  void DrawTestTriangle(float z, float angle = 0.f) {
#ifdef _DEBUG
    struct Vector3D {
      float x = 0.f;
      float y = 0.f;
      float z = 0.f;
    };

    struct VertexConstBuffer {
        DirectX::XMMATRIX transformation;
    };
    VertexConstBuffer const vcb = {
      DirectX::XMMatrixTranspose(
        DirectX::XMMatrixRotationZ(angle) * 
        DirectX::XMMatrixRotationX(angle) * 
        DirectX::XMMatrixTranslation(0.f, 0.f, z) * 
        DirectX::XMMatrixPerspectiveLH(1.f, 3.f / 4.f, 0.2f, 10.f)
        )
    };

    struct PixelConstBuffer {
      struct {
        float r;
        float g;
        float b;
        float a;
      } colors[6u];
    };
    PixelConstBuffer const pcb {
      {
        {1.f, 0.f, 0.f},
        {0.f, 1.f, 0.f},
        {0.f, 0.f, 1.f},
        {1.f, 1.f, 0.f},
        {0.f, 1.f, 1.f},
        {1.f, 0.f, 1.f}
      }
    };

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
        0u, 1u, 3u, 2u, 3u, 1u,
        3u, 2u, 7u, 6u, 7u, 2u,
        0u, 4u, 1u, 5u, 1u, 4u,
        6u, 5u, 7u, 4u, 7u, 5u,
        1u, 5u, 2u, 6u, 2u, 5u,
        4u, 0u, 7u, 3u, 7u, 0u
    };

    constexpr UINT kNoCpuAccessToBuffer = 0u;
    constexpr UINT kNoMisc = 0u;

    D3D11_BUFFER_DESC const vertex_buffer_conf{
      .ByteWidth = vertices.size() * sizeof(Vector3D),
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
    device_context_->IASetIndexBuffer(indices_buffer_ptr.Get(), DXGI_FORMAT_R16_UINT, 0u);

    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout{};
    constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 1u> input_element3d_descriptor{
      D3D11_INPUT_ELEMENT_DESC{
        .SemanticName = "Position",
        .SemanticIndex = 0u,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0u,
        .AlignedByteOffset = 0u,
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0u},
      };
    StartTraceInDebugMode();
    if (HRESULT const operation_status = device_->CreateInputLayout(
            input_element3d_descriptor.data(), 
            input_element3d_descriptor.size(),
            shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(), &input_layout);
        FAILED(operation_status)) {
      throw CreateDirectXError(
          operation_status, "Failed to create input layout",
          "Resource for Graphics was not allocated", __FILEW__, __LINE__);
    }

    device_context_->IASetInputLayout(input_layout.Get());
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
