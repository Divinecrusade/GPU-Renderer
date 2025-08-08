#ifndef FRAME_TIMER_HPP
#define FRAME_TIMER_HPP

#include <chrono>

namespace gpu_renderer {
class FrameTimer {
 private:
  using Clock = std::chrono::high_resolution_clock;

 public:
  using DeltaTime = float;

 public:
  DeltaTime Mark() noexcept {
    auto const prev_checkpoint{cur_checkpoint_};
    cur_checkpoint_ = Clock::now();
    return std::chrono::duration<DeltaTime>(cur_checkpoint_ - prev_checkpoint).count();
  }

 private:
  Clock::time_point cur_checkpoint_{Clock::now()};
};
}

#endif  // !DELTA_TIMER_HPP
