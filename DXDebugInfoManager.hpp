#ifndef DX_DEBUG_INFO_MANAGER_HPP
#define DX_DEBUG_INFO_MANAGER_HPP

#include "OptimisedDirect3dHeader.hpp"
#include "OptimisedStlHeader.hpp"

namespace gpu_renderer::debug {
class DXDebugInfoManager {
 public:
  DXDebugInfoManager() noexcept;
  DXDebugInfoManager(DXDebugInfoManager const&) = delete;
  DXDebugInfoManager(DXDebugInfoManager&&) = delete;

  DXDebugInfoManager& operator=(DXDebugInfoManager const&) = delete;
  DXDebugInfoManager& operator=(DXDebugInfoManager&&) = delete;

  ~DXDebugInfoManager() noexcept;

  void StartTrace();
  std::expected<std::wstring, std::wstring> GetTraceLog() const;

 private:
  HMODULE dxgi_debug_lib_{};

  UINT64 next_message_id_ = 0u;
  IDXGIInfoQueue* messages_queue_ = nullptr;
};
}

#endif  // !DXGI_DEBUG_INFO_MANAGER_HPP
