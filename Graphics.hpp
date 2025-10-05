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

  void DrawTestTriangle() {
#ifdef _DEBUG
    struct Vector2D {
      float x = 0.f;
      float y = 0.f;
    };
    std::array<Vector2D, 3u> triangle{
     Vector2D{.x = -0.5f, .y = -0.5f},
     Vector2D{.x = 0.f, .y = 0.5f},
     Vector2D{.x = 0.5f, .y = -0.5f}
    }; 

    constexpr UINT kNoCpuAccessToBuffer = 0u;
    constexpr UINT kNoMisc = 0u;
    D3D11_BUFFER_DESC const vertex_buffer_conf{
      .ByteWidth = triangle.size() * sizeof(Vector2D),
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_VERTEX_BUFFER,
      .CPUAccessFlags = kNoCpuAccessToBuffer,
      .MiscFlags = kNoMisc
    };
    D3D11_SUBRESOURCE_DATA const vertex_buffer{.pSysMem = triangle.data()};
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
    constexpr UINT stride = 0u;
    constexpr UINT offset = 0u;
    StartTraceInDebugMode();
    device_context_->IASetVertexBuffers(0u, 1u, vertex_buffers.GetAddressOf(),
                                        &stride, &offset);
    device_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device_context_->Draw(triangle.size(), 0u); 
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
  
#ifdef _DEBUG
  debug::DXDebugInfoManager debug_info_{};
#endif  // _DEBUG
};
}  // namespace gpu_renderer

#endif  // !GRAPHICS_HPP
