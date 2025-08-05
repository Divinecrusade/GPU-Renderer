#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include "OptimisedBoostHeader.hpp"
#include "OptimisedWindowsHeader.hpp"

#include <utility>
#include <optional>
#include <bitset>
#include <limits>
#include <numeric>
#include <string>
#include <queue>

namespace gpu_renderer {
class Keyboard {
 public:
  class View;

 private:
  friend View;
  
 public:
  using KeyCode = unsigned char;
  enum class EventType {
    kPressed, kReleased
  };
  using KeyEvent = std::pair<KeyCode, EventType>;
  
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

    bool IsKeyPressed(KeyCode key) const;
    bool IsKeyEventHappened() const;
    bool IsCharTyped() const;

    std::optional<KeyEvent> GetOldestKeyEvent() const;
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
  void ClearKeyEventsQueue() noexcept;
  void ClearCharsBuffer() noexcept;
 
 private:
  static constexpr std::size_t kKeyEventsQueueSize{256ull};
  static constexpr std::size_t kCharsBufferSize{256ull};

 private:
  bool auto_repeating_{false};

  std::bitset<std::numeric_limits<KeyCode>::max()> keys_state_{};
  std::queue<KeyEvent, boost::circular_buffer<KeyEvent>> key_events_queue_{
      boost::circular_buffer<KeyEvent>{kKeyEventsQueueSize}};
  boost::circular_buffer<wchar_t> chars_buffer_{kCharsBufferSize};
};
}

#endif  // !KEYBOARD_HPP
