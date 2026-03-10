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

  void DrawIndexed(UINT indices_count);

  void SetProjection(DirectX::XMMATRIX new_projection) noexcept;
  const DirectX::XMMATRIX& GetProjection() const noexcept;

 private:
  class ProtectiveLayer {
   private:
    friend Graphics;
    ProtectiveLayer() = default;
    ProtectiveLayer(ProtectiveLayer const&) = delete;
    ProtectiveLayer(ProtectiveLayer&&) = delete;

    ProtectiveLayer& operator=(ProtectiveLayer const&) = delete;
    ProtectiveLayer& operator=(ProtectiveLayer&&) = delete;

    ~ProtectiveLayer() = default;

   public:
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

    exception::DeviceRemovedError CreateDeviceRemovedError(ID3D11Device& removed_device,
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
#ifdef _DEBUG
    debug::DXDebugInfoManager debug_info_;
#endif  // _DEBUG
  };

 public:
  class Bindable {
   public:
    Bindable(Graphics& gfx) noexcept;
    virtual ~Bindable() = default;

    virtual void Activate() = 0;

   protected:
    ID3D11DeviceContext& GetDeviceContext() noexcept;
    ID3D11Device& GetDevice() noexcept;
    Graphics::ProtectiveLayer& GetDebugger() noexcept;

   private:
    Graphics const& gfx_;
  };
  friend class Bindable;

 private:
  Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_{};
  Microsoft::WRL::ComPtr<ID3D11Device> device_{};
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context_{};
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_{};
  Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_buf_view_{};
  DirectX::XMMATRIX projection_ = DirectX::XMMatrixIdentity();
  mutable ProtectiveLayer debugger_{};
};
}  // namespace gpu_renderer

#endif  // !GRAPHICS_HPP
