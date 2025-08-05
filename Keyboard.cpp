#include "Keyboard.hpp"

#include <cassert>
#include <gsl/gsl>
#include <iostream>

gpu_renderer::Keyboard::View::View(Keyboard& kbd) noexcept : kbd_{&kbd} {}

bool gpu_renderer::Keyboard::View::IsKeyPressed(KeyCode key) const noexcept {
  return gsl::at(kbd_->keys_state_, key);
}

bool gpu_renderer::Keyboard::View::IsKeyEventHappened() const noexcept {
  return !kbd_->key_events_queue_.empty();
}

bool gpu_renderer::Keyboard::View::IsCharTyped() const noexcept {
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

  std::wstring buffer_content{kbd_->chars_buffer_.begin(),
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
    try {
      std::wclog << L"EnableAutoRepeat method called, but auto repeating is "
                    L"already enabled\n";
    } catch (...) {
      OutputDebugStringW(
          L"Exception raised in log Keyboard EnableAutoRepeat\n");
    }
  }
#endif  // LOG_KEYBOARD
  auto_repeating_ = true;
}

void gpu_renderer::Keyboard::DisableAutoRepeat() noexcept {
#ifdef LOG_KEYBOARD
  if (auto_repeating_) {
    try {
      std::wclog << L"DisableAutoRepeat method called, but auto repeating is "
                    L"already disabled\n";
    } catch (...) {
      OutputDebugStringW(
          L"Exception raised in log Keyboard DisableAutoRepeat\n");
    }
  }
#endif  // LOG_KEYBOARD
  auto_repeating_ = false;
}

bool gpu_renderer::Keyboard::IsAutoRepeating() const noexcept {
  return auto_repeating_;
}

void gpu_renderer::Keyboard::OnKeyDown(KeyCode key) {
#ifdef LOG_KEYBOARD
  try {
    std::wclog << L"Key 0x" << std::hex << key << L" pressed\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Keyboard OnKeyDown\n");
  }
#endif  // LOG_KEYBOARD
  keys_state_.set(key);
  key_events_queue_.push(std::make_pair(key, EventType::kPressed));
}

void gpu_renderer::Keyboard::OnKeyUp(KeyCode key) {
#ifdef LOG_KEYBOARD
  try {
    std::wclog << L"Key 0x" << std::hex << key << L" released\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Keyboard OnKeyUp\n");
  }
#endif  // LOG_KEYBOARD
  keys_state_.set(key, false);
  key_events_queue_.push(std::make_pair(key, EventType::kReleased));
}

void gpu_renderer::Keyboard::OnChar(wchar_t symbol) {
#ifdef LOG_KEYBOARD
  try {
    std::wclog << L"Char '" << symbol << L"' typed\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Keyboard OnChar\n");
  }
#endif  // LOG_KEYBOARD
  chars_buffer_.push_back(symbol);
}

void gpu_renderer::Keyboard::ClearKeysState() noexcept { keys_state_.reset(); }

void gpu_renderer::Keyboard::ClearKeyEventsQueue() {
  std::queue<KeyEvent, boost::circular_buffer<KeyEvent>> empty_key_events_queue{
      boost::circular_buffer<KeyEvent>{kKeyEventsQueueSize}};
  key_events_queue_.swap(empty_key_events_queue);
}

void gpu_renderer::Keyboard::ClearCharsBuffer() noexcept {
  chars_buffer_.clear();
}
