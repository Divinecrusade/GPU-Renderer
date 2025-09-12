#ifndef DEVICE_REMOVED_ERROR_HPP
#define DEVICE_REMOVED_ERROR_HPP

#include "OptimisedDirect3dHeader.hpp"
#include "DirectXError.hpp"
#include "DXDebugInfoManager.hpp"

namespace gpu_renderer::exception {
class DeviceRemovedError : public DirectXError {
 public:
#ifdef _DEBUG
  DeviceRemovedError(wchar_t const* file, int line, char const* message,
                     ID3D11Device& removed_device);
  DeviceRemovedError(wchar_t const* file, int line, char const* message,
                     ID3D11Device& removed_device,
                     debug::DXDebugInfoManager const& debug_tracer);
#endif  // _DEBUG
  DeviceRemovedError(char const* message, ID3D11Device& removed_device);
  
  [[nodiscard]] std::wstring WhatHappened() const override;

  [[nodiscard]] static constexpr bool IsDeviceRemovedErrorHappened(
                                          HRESULT operation_status) noexcept {
    return operation_status == DXGI_ERROR_DEVICE_REMOVED;
  }

 private:
  HRESULT remove_reason{};
};
}  // namespace gpu_renderer::exception

#endif  // !DEVICE_REMOVED_ERROR_HPP
