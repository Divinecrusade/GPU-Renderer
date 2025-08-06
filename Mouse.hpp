#ifndef MOUSE_HPP
#define MOUSE_HPP

#include <utility>
#include <optional>
#include <queue>

#include "OptimisedBoostHeader.hpp"
#include "OptimisedWindowsHeader.hpp"

namespace gpu_renderer {
class Mouse final {
 public:
  class View;

 private:
  friend View;

 public:
  enum class EventType {
    kLeftButtonPressed,
    kLeftButtonReleased,
    kRightButtonPressed,
    kRightButtonReleased,
    kWheelUp,
    kWheelDown,
    kMove,
    kHoverWindow,
    kLeaveWindow
  };
  using Event = std::pair<POINTS, EventType>;

  class View {
   public:
    View() = delete;
    View(Mouse& mse) noexcept;
    View(View const&) = default;
    View(View&&) = default;

    View& operator=(View const&) = default;
    View& operator=(View&&) = default;

    ~View() = default;

    friend bool operator==(View const& lhs, View const& rhs) noexcept;
    friend bool operator!=(View const& lhs, View const& rhs) noexcept;

    bool IsLeftButtonPressed() const noexcept;
    bool IsRightButtonPressed() const noexcept;
    bool IsInWindow() const noexcept;

    std::optional<Event> GetOldestEvent() const;

   private:
    Mouse* mse_;
  };

 public:
  static constexpr std::size_t kDefaultEventsQueueSize{512ull};

 public:
  Mouse() = delete;
  Mouse(std::size_t events_queue_size);
  Mouse(Mouse const&) = delete;
  Mouse(Mouse&&) = delete;

  Mouse& operator=(Mouse const&) = delete;
  Mouse& operator=(Mouse&&) = delete;

  ~Mouse() = default;

  void OnLButtonDown(LPARAM lParam);
  void OnLButtonUp(LPARAM lParam);
  void OnRButtonDown(LPARAM lParam);
  void OnRButtonUp(LPARAM lParam);
  void OnMove(LPARAM lParam);
  void OnWheel(LPARAM lParam, WPARAM wParam);

  void OnHoverWindow(LPARAM lParam);
  void OnLeaveWindow(LPARAM lParam);

 private:
  std::queue<Event, boost::circular_buffer<Event>> events_queue_;
  bool left_button_pressed_{false};
  bool right_button_pressed_{false};
#pragma warning(push)
#pragma warning(disable : 4820)
  bool in_window_{false};
#pragma warning(pop)
  int accumulated_wheel_delta_{0};
};
} // namespace gpu_renderer

#endif  // !MOUSE_HPP
