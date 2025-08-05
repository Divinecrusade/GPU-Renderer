#include "Keyboard.hpp"

#include <cassert>
#include <iostream>

gpu_renderer::Keyboard::View::View(Keyboard& kbd) noexcept
: kbd_{&kbd} {}

bool gpu_renderer::Keyboard::View::IsKeyPressed(KeyCode key) const {
  return kbd_->keys_state_[key];
}

bool gpu_renderer::Keyboard::View::IsKeyEventHappened() const { 
  return !kbd_->key_events_queue_.empty(); 
}

bool gpu_renderer::Keyboard::View::IsCharTyped() const { 
  return !kbd_->chars_buffer_.empty(); 
}

std::optional<gpu_renderer::Keyboard::KeyEvent>
gpu_renderer::Keyboard::View::GetOldestKeyEvent() const {
  if (kbd_->key_events_queue_.empty()) return std::nullopt;

  KeyEvent const key_event{std::move(kbd_->key_events_queue_.front())};
  kbd_->key_events_queue_.pop();
  return key_event;
}

std::optional<wchar_t> gpu_renderer::Keyboard::View::GetFirstChar() const {
  if (kbd_->chars_buffer_.empty()) return std::nullopt;

  auto const symbol{std::move(kbd_->chars_buffer_.front())};
  kbd_->chars_buffer_.pop_front();
  return symbol;
}

std::wstring gpu_renderer::Keyboard::View::GetChars() const {
  if (kbd_->chars_buffer_.empty()) return L"";
  std::wstring buffer_content {kbd_->chars_buffer_.begin(), 
                               kbd_->chars_buffer_.end()};
  kbd_->chars_buffer_.clear();
  return buffer_content;
}

bool gpu_renderer::operator==(Keyboard::View const& lhs,
                              Keyboard::View const& rhs) noexcept {
  return lhs.kbd_ == rhs.kbd_;
}

bool gpu_renderer::operator!=(Keyboard::View const& lhs,
                              Keyboard::View const& rhs) noexcept {
  return !(lhs == rhs);
}

void gpu_renderer::Keyboard::EnableAutoRepeat() noexcept {
#ifdef LOG_KEYBOARD
  if (auto_repeating_) {
    std::wclog << L"EnableAutoRepeat method called, but auto repeating is already enabled\n";
  }
#endif  // LOG_KEYBOARD
  auto_repeating_ = true;
}

void gpu_renderer::Keyboard::DisableAutoRepeat() noexcept {
#ifdef LOG_KEYBOARD
  if (auto_repeating_) {
    std::wclog << L"DisableAutoRepeat method called, but auto repeating is "
                  L"already disabled\n";
  }
#endif  // LOG_KEYBOARD
  auto_repeating_ = false;
}

bool gpu_renderer::Keyboard::IsAutoRepeating() const noexcept {
  return auto_repeating_;
}

void gpu_renderer::Keyboard::OnKeyDown(KeyCode key) {
#ifdef LOG_KEYBOARD
  std::wclog << L"Key 0x" << std::hex << key << L" pressed\n";
#endif  // LOG_KEYBOARD
  keys_state_.set(key);
  key_events_queue_.push(std::make_pair(key, EventType::kPressed));
}

void gpu_renderer::Keyboard::OnKeyUp(KeyCode key) {
#ifdef LOG_KEYBOARD
  std::wclog << L"Key 0x" << std::hex << key << L" released\n";
#endif  // LOG_KEYBOARD
  keys_state_.set(key, false);
  key_events_queue_.push(std::make_pair(key, EventType::kReleased));
}

void gpu_renderer::Keyboard::OnChar(wchar_t symbol) {
#ifdef LOG_KEYBOARD
  std::wclog << L"Char '" << symbol << L"' typed\n";
#endif  // LOG_KEYBOARD
  chars_buffer_.push_back(symbol);
}

void gpu_renderer::Keyboard::ClearKeysState() noexcept { 
  keys_state_.reset(); 
}

void gpu_renderer::Keyboard::ClearKeyEventsQueue() noexcept {
  std::queue<KeyEvent, boost::circular_buffer<KeyEvent>> empty_key_events_queue{
    boost::circular_buffer<KeyEvent>{kKeyEventsQueueSize}};
  key_events_queue_.swap(empty_key_events_queue);
}

void gpu_renderer::Keyboard::ClearCharsBuffer() noexcept {
  chars_buffer_.clear();
}
