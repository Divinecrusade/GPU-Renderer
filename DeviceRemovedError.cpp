#include "DeviceRemovedError.hpp"

namespace gpu_renderer::exception {
#ifdef _DEBUG
DeviceRemovedError::DeviceRemovedError(wchar_t const* file, int line,
                                       char const* message,
                                       ID3D11Device& removed_device)
    : DirectXError{file, line, message, DXGI_ERROR_DEVICE_REMOVED},
      remove_reason{removed_device.GetDeviceRemovedReason()} {}
#endif  // _DEBUG

DeviceRemovedError::DeviceRemovedError(char const* message,
                                       ID3D11Device& removed_device)
    : DirectXError{message, DXGI_ERROR_DEVICE_REMOVED},
      remove_reason{removed_device.GetDeviceRemovedReason()} {}

std::wstring DeviceRemovedError::WhatHappened() const {
  return DirectXError::WhatHappened() + DXGetErrorStringW(remove_reason);
}
}
