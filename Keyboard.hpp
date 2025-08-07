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
class Keyboard final {
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

    [[nodiscard]] bool IsKeyPressed(KeyCode key) const noexcept;
    [[nodiscard]] bool IsKeyEventHappened() const noexcept;
    [[nodiscard]] bool IsCharTyped() const noexcept;

    std::optional<Event> GetOldestEvent() const;
    std::optional<wchar_t> GetFirstChar() const;
    std::wstring GetChars() const;

   private:
    Keyboard* kbd_;
  };

 public:
  static constexpr std::size_t kDefaultEventsQueueSize{128ull};
  static constexpr std::size_t kDefaultCharsBufferSize{64ull};

 public:
  Keyboard() = delete;
  Keyboard(std::size_t events_queue_size, 
           std::size_t chars_buffer_size);
  Keyboard(Keyboard const&) = delete;
  Keyboard(Keyboard&&) = delete;

  Keyboard& operator=(Keyboard const&) = delete;
  Keyboard& operator=(Keyboard&&) = delete;

  ~Keyboard() = default;

  void EnableAutoRepeat() noexcept;
  void DisableAutoRepeat() noexcept;
  bool IsAutoRepeating() const noexcept;

  void OnKeyDown(KeyCode key);
  void OnKeyUp(KeyCode key);
  void OnChar(wchar_t symbol);

  void ClearKeysState() noexcept;
  void ClearEventsQueue();
  void ClearCharsBuffer() noexcept;

 private:
#pragma warning(push)
#pragma warning(disable : 4820)
  bool auto_repeating_{false};
#pragma warning(pop)
  std::bitset<std::numeric_limits<KeyCode>::max() + 1> keys_state_{};

  std::queue<Event, boost::circular_buffer<Event>> events_queue_;
  boost::circular_buffer<wchar_t> chars_buffer_;
};
}  // namespace gpu_renderer

#endif  // !KEYBOARD_HPP
