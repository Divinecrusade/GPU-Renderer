#ifndef GPU_TIMER_HPP
#define GPU_TIMER_HPP

#include "../Graphics.hpp"

namespace gpu_renderer::profiling {
// Замер времени выполнения команд на GPU через запросы D3D11_QUERY_TIMESTAMP.
// Использование: Begin -> команды отрисовки -> End -> (Present) -> WaitMilliseconds.
class GpuTimer final {
 public:
  explicit GpuTimer(Graphics& gfx);

  void Begin(Graphics& gfx);
  void End(Graphics& gfx);

  // Блокирует CPU до готовности результатов. nullopt — интервал недостоверен
  // (disjoint: например, изменилась частота GPU), такой кадр нужно отбросить.
  [[nodiscard]] std::optional<double> WaitMilliseconds(Graphics& gfx);

 private:
  Microsoft::WRL::ComPtr<ID3D11Query> disjoint_{};
  Microsoft::WRL::ComPtr<ID3D11Query> begin_{};
  Microsoft::WRL::ComPtr<ID3D11Query> end_{};
};
}  // namespace gpu_renderer::profiling

#endif  // !GPU_TIMER_HPP
