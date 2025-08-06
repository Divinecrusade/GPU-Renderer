#include "Mouse.hpp"

#include <algorithm>
#include <iostream>

gpu_renderer::Mouse::View::View(Mouse& mse) noexcept : mse_{&mse} {}

bool gpu_renderer::Mouse::View::IsLeftButtonPressed() const noexcept {
  return mse_->left_button_pressed_;
}

bool gpu_renderer::Mouse::View::IsRightButtonPressed() const noexcept {
  return mse_->right_button_pressed_;
}

bool gpu_renderer::Mouse::View::IsInWindow() const noexcept { 
  return mse_->in_window_; 
}

std::optional<gpu_renderer::Mouse::Event>
gpu_renderer::Mouse::View::GetOldestEvent() const {
  if (mse_->events_queue_.empty()) return std::nullopt;

  Event const key_event{std::move(mse_->events_queue_.front())};
  mse_->events_queue_.pop();
  return key_event;
}

bool gpu_renderer::operator==(gpu_renderer::Mouse::View const& lhs, 
                              gpu_renderer::Mouse::View const& rhs) noexcept {
  return lhs.mse_ == rhs.mse_;
}

bool gpu_renderer::operator!=(gpu_renderer::Mouse::View const& lhs, 
                              gpu_renderer::Mouse::View const& rhs) noexcept {
  return !(lhs == rhs);
}

void gpu_renderer::Mouse::OnLButtonDown(LPARAM lParam) {
#ifdef LOG_MOUSE
  try {
    auto const pos{MAKEPOINTS(lParam)};
    std::wclog << L"Mouse left button pressed. Current position: (" 
               << pos.x << L", " << pos.y << L")\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Mouse OnLButtonDown\n");
  }
#endif  // LOG_MOUSE
  events_queue_.push(
      std::make_pair(MAKEPOINTS(lParam), EventType::kLeftButtonPressed));
  left_button_pressed_ = true;
}

void gpu_renderer::Mouse::OnLButtonUp(LPARAM lParam) {
#ifdef LOG_MOUSE
  try {
    auto const pos{MAKEPOINTS(lParam)};
    std::wclog << L"Mouse left button released. Current position: (" 
               << pos.x
               << L", " << pos.y << L")\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Mouse OnLButtonUp\n");
  }
#endif  // LOG_MOUSE
  events_queue_.push(
      std::make_pair(MAKEPOINTS(lParam), EventType::kLeftButtonReleased));
  left_button_pressed_ = false;
}

void gpu_renderer::Mouse::OnRButtonDown(LPARAM lParam) {
#ifdef LOG_MOUSE
  try {
    auto const pos{MAKEPOINTS(lParam)};
    std::wclog << L"Mouse right button pressed. Current position: (" 
               << pos.x
               << L", " << pos.y << L")\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Mouse OnRButtonDown\n");
  }
#endif  // LOG_MOUSE
  events_queue_.push(
      std::make_pair(MAKEPOINTS(lParam), EventType::kRightButtonPressed));
  right_button_pressed_ = true;
}

void gpu_renderer::Mouse::OnRButtonUp(LPARAM lParam) {
#ifdef LOG_MOUSE
  try {
    auto const pos{MAKEPOINTS(lParam)};
    std::wclog << L"Mouse right button released. Current position: (" 
               << pos.x
               << L", " << pos.y << L")\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Mouse OnRButtonUp\n");
  }
#endif  // LOG_MOUSE
  events_queue_.push(
      std::make_pair(MAKEPOINTS(lParam), EventType::kRightButtonReleased));
  right_button_pressed_ = false;
}

void gpu_renderer::Mouse::OnMove(LPARAM lParam) {
#ifdef LOG_MOUSE
  auto const pos{MAKEPOINTS(lParam)};
  try {
    std::wclog << L"Mouse moved. Current position: (" 
               << pos.x << L", " << pos.y
               << L")\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Mouse OnMove\n");
  }
#endif  // LOG_MOUSE
  events_queue_.push(std::make_pair(MAKEPOINTS(lParam), EventType::kMove));
}

void gpu_renderer::Mouse::OnWheel(LPARAM lParam, WPARAM wParam) {
  auto const pos{MAKEPOINTS(lParam)};
  accumulated_wheel_delta_ += GET_WHEEL_DELTA_WPARAM(wParam);
#ifdef LOG_MOUSE
  try {
    std::wclog << L"Mouse wheeled. Accumulated delta: " << accumulated_wheel_delta_ 
               <<L"\nCurrent position: (" << pos.x << L", " << pos.y << L")\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Mouse OnWheel\n");
  }
#endif  // LOG_MOUSE
#ifdef LOG_MOUSE
  int i{0};
#endif  // LOG_MOUSE
  if (accumulated_wheel_delta_ < 0) {
#ifdef LOG_MOUSE
      try {
        std::wclog << L"Mouse wheel rolled down\n";
      } catch (...) {
        OutputDebugStringW(L"Exception raised in log Mouse OnWheel\n");
      }
#endif  // LOG_MOUSE
    while (accumulated_wheel_delta_ <= -WHEEL_DELTA) {
      events_queue_.push(std::make_pair(pos, EventType::kWheelDown));
      accumulated_wheel_delta_ += WHEEL_DELTA;
#ifdef LOG_MOUSE
      ++i;
#endif  // LOG_MOUSE
    }
  } else if (accumulated_wheel_delta_ > 0) {
#ifdef LOG_MOUSE
    try {
      std::wclog << L"Mouse wheel rolled up\n";
    } catch (...) {
      OutputDebugStringW(L"Exception raised in log Mouse OnWheel\n");
    }
#endif  // LOG_MOUSE
    while (accumulated_wheel_delta_ >= WHEEL_DELTA) {
      events_queue_.push(std::make_pair(pos, EventType::kWheelUp));
      accumulated_wheel_delta_ -= WHEEL_DELTA;
#ifdef LOG_MOUSE
      ++i;
#endif  // LOG_MOUSE
    }
  }
#ifdef LOG_MOUSE
  try {
    std::wclog << L"Generated: " << i << L" events\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Mouse OnWheel\n");
  }
#endif  // LOG_MOUSE
}

void gpu_renderer::Mouse::OnHoverWindow(LPARAM lParam) {
#ifdef LOG_MOUSE
  try {
    auto const pos{MAKEPOINTS(lParam)};
    std::wclog << L"Mouse entered window. Current position: (" << pos.x << L", "
               << pos.y << L")\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Mouse OnHoverWindow\n");
  }
#endif  // LOG_MOUSE
  events_queue_.push(
      std::make_pair(MAKEPOINTS(lParam), EventType::kHoverWindow));
  in_window_ = true;
}

void gpu_renderer::Mouse::OnLeaveWindow(LPARAM lParam) {
#ifdef LOG_MOUSE
  try {
    auto const pos{MAKEPOINTS(lParam)};
    std::wclog << L"Mouse left window. Current position: (" 
               << pos.x << L", " << pos.y << L")\n";
  } catch (...) {
    OutputDebugStringW(L"Exception raised in log Mouse OnLeaveWindow\n");
  }
#endif  // LOG_MOUSE
  events_queue_.push(
      std::make_pair(MAKEPOINTS(lParam), EventType::kLeaveWindow));
  in_window_ = false;
}

