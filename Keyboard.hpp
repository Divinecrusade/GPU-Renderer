#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <bitset>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <string>
#include <utility>

#include "OptimisedBoostHeader.hpp"
#include "OptimisedWindowsHeader.hpp"

namespace gpu_renderer {
class Keyboard {
 public:
  class View;

 private:
  friend View;

 public:
  using KeyCode = unsigned char;
  enum class EventType { kPressed, kReleased };
  using Event = std::pair<KeyCode, EventType>;

  class View {
   public:
    View() = delete;
    View(Keyboard& kbd) noexcept;
    View(View const&) = default;
    View(View&&) = default;

    View& operator=(View const&) = default;
    View& operator=(View&&) = default;

    ~View() = default;

    friend bool operator==(View const& lhs, View const& rhs) noexcept;
    friend bool operator!=(View const& lhs, View const& rhs) noexcept;

    bool IsKeyPressed(KeyCode key) const noexcept;
    bool IsKeyEventHappened() const noexcept;
    bool IsCharTyped() const noexcept;

    std::optional<Event> GetOldestEvent() const;
    std::optional<wchar_t> GetFirstChar() const;
    std::wstring GetChars() const;

   private:
    Keyboard* kbd_;
  };

 public:
  Keyboard() = default;
  Keyboard(Keyboard const&) = delete;
  Keyboard(Keyboard&&) = delete;

  Keyboard& operator=(Keyboard const&) = delete;
  Keyboard& operator=(Keyboard&&) = delete;

  virtual ~Keyboard() = default;

  void EnableAutoRepeat() noexcept;
  void DisableAutoRepeat() noexcept;
  bool IsAutoRepeating() const noexcept;

  void OnKeyDown(KeyCode key);
  void OnKeyUp(KeyCode key);
  void OnChar(wchar_t symbol);

  void ClearKeysState() noexcept;
  void ClearKeyEventsQueue();
  void ClearCharsBuffer() noexcept;

 private:
  static constexpr std::size_t kKeyEventsQueueSize{256ull};
  static constexpr std::size_t kCharsBufferSize{256ull};

 private:
#pragma warning(push)
#pragma warning(disable : 4820)
  bool auto_repeating_{false};
#pragma warning(pop)
  std::bitset<std::numeric_limits<KeyCode>::max() + 1> keys_state_{};
  std::queue<Event, boost::circular_buffer<Event>> key_events_queue_{
      boost::circular_buffer<Event>{kKeyEventsQueueSize}};
  boost::circular_buffer<wchar_t> chars_buffer_{kCharsBufferSize};
};
}  // namespace gpu_renderer

#endif  // !KEYBOARD_HPP
