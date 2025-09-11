#include "DXDebugInfoManager.hpp"

#include "WinError.hpp"
#include "DirectXError.hpp"

#pragma comment(lib, "dxguid")

namespace gpu_renderer::debug {
DXDebugInfoManager::DXDebugInfoManager() 
: dxgi_debug_lib_{LoadLibraryExW(L"dxgidebug.dll", NULL,
                                 LOAD_LIBRARY_SEARCH_SYSTEM32)} {
  if (!dxgi_debug_lib_) {
#ifdef _DEBUG
    throw exception::WinError{__FILEW__, __LINE__, "DXGI debug dll wasn't loaded",
                              GetLastError()};
#else
    throw exception::WinError{"Debug library was not loaded",
                              GetLastError()};
#endif  // _DEBUG
  }

#pragma warning(push)
#pragma warning(disable : 26490 4191)
  auto const get_debug_interface = reinterpret_cast<decltype(DXGIGetDebugInterface)*>(
      GetProcAddress(dxgi_debug_lib_, "DXGIGetDebugInterface"));
#pragma warning(pop)
  if (!get_debug_interface) {
    std::ignore = FreeLibrary(dxgi_debug_lib_);
#ifdef _DEBUG
    throw exception::WinError{__FILEW__, __LINE__,
                              "DXGIGetDebugInterface wasn't found", GetLastError()};
#else
    throw exception::WinError{"Debug function was not founded", GetLastError()};
#endif  // _DEBUG
  }

#pragma warning(push)
#pragma warning(disable : 26490)
  if (HRESULT const operation_status =
          get_debug_interface(__uuidof(IDXGIInfoQueue), reinterpret_cast<void**>(&messages_queue_));
     FAILED(operation_status)) {
#pragma warning(pop)
    std::ignore = FreeLibrary(dxgi_debug_lib_);
#ifdef _DEBUG
    throw exception::DirectXError{__FILEW__, __LINE__,
                                  "Failed to access debug info queue",
                                  operation_status};
#else
    throw exception::DirectXError{"Failed to access debug info",
                                  operation_status};
#endif  // _DEBUG
  }
  assert(messages_queue_ != nullptr);
}

DXDebugInfoManager::~DXDebugInfoManager() noexcept {
  std::ignore = FreeLibrary(dxgi_debug_lib_);
  try {
    std::ignore = messages_queue_->Release();
  } catch (...) {
    OutputDebugStringW(L"Exception raised during messages_queue_ releasing in destructor of DXDebugInfoManager\n");
  }
}

void DXDebugInfoManager::StartTrace() {
  next_message_id_ = messages_queue_->GetNumStoredMessages(DXGI_DEBUG_ALL);
}

std::wstring DXDebugInfoManager::GetTraceLog() const {
  std::wostringstream log{};
  UINT64 const end = messages_queue_->GetNumStoredMessages(DXGI_DEBUG_ALL);
  for (UINT64 cur = next_message_id_; cur != end; ++cur) {
    assert(cur < end);
    __assume(cur < end);

    SIZE_T message_size{};
    if (HRESULT const operation_status = messages_queue_->GetMessageW(
            DXGI_DEBUG_ALL, cur, nullptr, &message_size);
        FAILED(operation_status)) {
#ifdef _DEBUG
      throw exception::DirectXError{__FILEW__, __LINE__,
                                    "Failed to determine message length from debug queue",
                                    operation_status};
#else
      throw exception::DirectXError{"Failed to retrieve debug message",
                                    operation_status};
#endif  // _DEBUG
    }
#pragma warning(push)
#pragma warning(disable : 26414)
    auto message_buffer = std::make_unique<std::byte[]>(message_size);
#pragma warning(pop)
#pragma warning(push)
#pragma warning(disable : 26490)
    auto message = reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(
        message_buffer.get());
#pragma warning(pop)
    if (HRESULT const operation_status = messages_queue_->GetMessageW(
            DXGI_DEBUG_ALL, cur, message,
            &message_size);
        FAILED(operation_status)) {
#ifdef _DEBUG
      throw exception::DirectXError{
          __FILEW__, __LINE__,
          "Failed to determine message length from debug queue",
          operation_status};
#else
      throw exception::DirectXError{"Failed to retrieve debug message",
                                    operation_status};
#endif  // _DEBUG
    }
    log << message->pDescription << std::endl;
  }
  return log.str();
}
}