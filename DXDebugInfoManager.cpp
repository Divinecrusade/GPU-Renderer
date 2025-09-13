#include "DXDebugInfoManager.hpp"

#include "WinError.hpp"
#include "DirectXError.hpp"

#pragma comment(lib, "dxguid")

namespace gpu_renderer::debug {
DXDebugInfoManager::DXDebugInfoManager() noexcept 
: dxgi_debug_lib_{LoadLibraryExW(L"dxgidebug.dll", NULL,
                                 LOAD_LIBRARY_SEARCH_SYSTEM32)} {
  try {
    if (!dxgi_debug_lib_) {
      throw exception::WinError::CreateFromGetLastError(
          "DXGI debug dll wasn't loaded", 
          "Debug library was not loaded",
          __FILEW__, __LINE__);
    }

#pragma warning(push)
#pragma warning(disable : 26490 4191)
    auto const get_debug_interface = reinterpret_cast<decltype(DXGIGetDebugInterface)*>(
        GetProcAddress(dxgi_debug_lib_, "DXGIGetDebugInterface"));
#pragma warning(pop)
    if (!get_debug_interface) {
      std::ignore = FreeLibrary(dxgi_debug_lib_);
      dxgi_debug_lib_ = NULL;
      throw exception::WinError::CreateFromGetLastError(
          "DXGIGetDebugInterface wasn't found",
          "Debug function was not founded",
          __FILEW__, __LINE__);
    }

#pragma warning(push)
#pragma warning(disable : 26490)
    if (HRESULT const operation_status =
            get_debug_interface(__uuidof(IDXGIInfoQueue), reinterpret_cast<void**>(&messages_queue_));
       FAILED(operation_status)) {
#pragma warning(pop)
      std::ignore = FreeLibrary(dxgi_debug_lib_);
      dxgi_debug_lib_ = NULL;
      throw exception::DirectXError::Create(
          operation_status,
          "Failed to access debug info queue",
          "Failed to access debug info",
          __FILEW__, __LINE__);
    }
    assert(messages_queue_ != nullptr);
  }
  catch (exception::SystemError const& e) {
    try {
      OutputDebugStringW(L"Exception raised in DXDebugInfoManager constructor\n");
      std::wcerr << L"Exception raised in DXDebugInfoManager constructor. " 
                 << e.GetTypeOfException() << L": " << e.WhatHappened();
    }
    catch (...) {
      OutputDebugStringW(L"Failed to log in console exception raised in DXDebugInfoManager constructor\n");
    }
  }
  catch (...) {
    OutputDebugStringW(L"Something went wrong in DXDebugInfoManager constructor\n");
  }
}

DXDebugInfoManager::~DXDebugInfoManager() noexcept {
  if (dxgi_debug_lib_) { 
    std::ignore = FreeLibrary(dxgi_debug_lib_);
  }
  try {
    if (messages_queue_) {
      std::ignore = messages_queue_->Release();
    }
  } catch (...) {
    OutputDebugStringW(L"Exception raised during messages_queue_ releasing in destructor of DXDebugInfoManager\n");
  }
}

void DXDebugInfoManager::StartTrace() {
  if (!messages_queue_) {
    OutputDebugStringW(L"SetTrace called on incorrect state of DXDebugInfoManager object\n");
    try {
      std::wcerr << L"SetTrace called on incorrect state of DXDebugInfoManager object\n";
    } catch (...) {
      OutputDebugStringW(L"Failed to log in console in DXDebugInfoManager StartTrace\n");
    }
    return;
  }
  
  next_message_id_ = messages_queue_->GetNumStoredMessages(DXGI_DEBUG_ALL);
}

std::wstring DXDebugInfoManager::GetTraceLog() const {
  if (!messages_queue_) {
    OutputDebugStringW(L"GetTraceLog called on incorrect state of DXDebugInfoManager object\n");
    try {
      std::wcerr << L"GetTraceLog called on incorrect state of DXDebugInfoManager object\n";
    } catch (...) {
      OutputDebugStringW(L"Failed to log in console in DXDebugInfoManager GetTraceLog\n");
    }
    return L"Unable to get info from dxgidebug.dll\n";
  }

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