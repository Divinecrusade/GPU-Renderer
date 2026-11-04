#include "GpuTimer.hpp"

#include <thread>

namespace gpu_renderer::profiling {
namespace {
template <typename Data>
HRESULT WaitQuery(ID3D11DeviceContext& context, ID3D11Query* query, Data& data) {
  HRESULT status = S_FALSE;
  while ((status = context.GetData(query, &data, sizeof(Data), 0u)) == S_FALSE) {
    std::this_thread::yield();
  }
  return status;
}
}  // namespace

GpuTimer::GpuTimer(Graphics& gfx) {
  D3D11_QUERY_DESC const disjoint_desc{.Query = D3D11_QUERY_TIMESTAMP_DISJOINT, .MiscFlags = 0u};
  D3D11_QUERY_DESC const timestamp_desc{.Query = D3D11_QUERY_TIMESTAMP, .MiscFlags = 0u};

  // ComPtr перегружает operator&, поэтому адреса берутся через GetAddressOf().
  for (auto [desc, query] : {std::pair{&disjoint_desc, disjoint_.GetAddressOf()},
                             std::pair{&timestamp_desc, begin_.GetAddressOf()},
                             std::pair{&timestamp_desc, end_.GetAddressOf()}}) {
    gfx.debugger_.StartTraceInDebugMode();
    if (HRESULT const operation_status = gfx.device_->CreateQuery(desc, query);
        FAILED(operation_status)) {
      throw gfx.debugger_.CreateDirectXError(operation_status, "GPU timestamp query was not created",
                                             "Profiler was not created", __FILEW__, __LINE__);
    }
  }
}

void GpuTimer::Begin(Graphics& gfx) {
  gfx.device_context_->Begin(disjoint_.Get());
  gfx.device_context_->End(begin_.Get());
}

void GpuTimer::End(Graphics& gfx) {
  gfx.device_context_->End(end_.Get());
  gfx.device_context_->End(disjoint_.Get());
}

std::optional<double> GpuTimer::WaitMilliseconds(Graphics& gfx) {
  auto& context = *gfx.device_context_.Get();
  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
  UINT64 begin_ticks = 0u;
  UINT64 end_ticks = 0u;
  for (HRESULT const operation_status :
       {WaitQuery(context, disjoint_.Get(), disjoint), WaitQuery(context, begin_.Get(), begin_ticks),
        WaitQuery(context, end_.Get(), end_ticks)}) {
    if (FAILED(operation_status)) {
      throw gfx.debugger_.CreateDirectXError(operation_status, "GPU timestamp query data was not read",
                                             "Profiling failed", __FILEW__, __LINE__);
    }
  }
  if (disjoint.Disjoint || disjoint.Frequency == 0u || end_ticks < begin_ticks) {
    return std::nullopt;
  }
  return static_cast<double>(end_ticks - begin_ticks) * 1000.0 / static_cast<double>(disjoint.Frequency);
}
}  // namespace gpu_renderer::profiling
