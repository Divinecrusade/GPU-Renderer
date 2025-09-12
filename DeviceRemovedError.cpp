#include "DeviceRemovedError.hpp"

namespace gpu_renderer::exception {
#ifdef _DEBUG
DeviceRemovedError::DeviceRemovedError(wchar_t const* file, int line,
                                       char const* message,
                                       ID3D11Device& removed_device)
    : DirectXError{file, line, message, DXGI_ERROR_DEVICE_REMOVED},
      remove_reason{removed_device.GetDeviceRemovedReason()} {
  assert(((void)"Exception must be created from device removed failed code status",
         remove_reason != S_OK));
}

DeviceRemovedError::DeviceRemovedError(wchar_t const* file, int line, 
                                       char const* message,
                                       ID3D11Device& removed_device,
                                       debug::DXDebugInfoManager const& debug_tracer)
    : DirectXError{file, line, message, DXGI_ERROR_DEVICE_REMOVED, debug_tracer},
      remove_reason{removed_device.GetDeviceRemovedReason()} {
  assert(((void)"Exception must be created from device removed failed code status",
          remove_reason != S_OK));
}
#endif  // _DEBUG

DeviceRemovedError::DeviceRemovedError(char const* message,
                                       ID3D11Device& removed_device)
    : DirectXError{message, DXGI_ERROR_DEVICE_REMOVED},
      remove_reason{removed_device.GetDeviceRemovedReason()} {
  assert(((void)"Exception must be created from device removed failed code status",
          remove_reason != S_OK));      
}

std::wstring DeviceRemovedError::WhatHappened() const {
  return DirectXError::WhatHappened() + DXGetErrorStringW(remove_reason);
}
}
