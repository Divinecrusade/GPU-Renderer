#include "pch.h"
#include "Mouse.hpp"

namespace input_handling {
using gpu_renderer::input::Mouse;

class MouseTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mouse_ = std::make_unique<Mouse>(Mouse::kDefaultEventsQueueSize);
    view_ = std::make_unique<Mouse::View>(*mouse_);
  }

  void TearDown() override {
    view_.reset();
    mouse_.reset();
  }

  std::unique_ptr<Mouse> mouse_;
  std::unique_ptr<Mouse::View> view_;

  static constexpr POINTS kOriginPos{0, 0};
  static constexpr POINTS kCenterPos{320, 240};
  static constexpr POINTS kBottomRightPos{640, 480};
  static constexpr POINTS kNegativePos{-10, -10};
};

TEST_F(MouseTest, DefaultConstructorCreatesValidMouse) {
  EXPECT_FALSE(view_->IsLeftButtonPressed());
  EXPECT_FALSE(view_->IsRightButtonPressed());
  EXPECT_FALSE(view_->IsInWindow());
  EXPECT_FALSE(view_->GetOldestEvent().has_value());
}

TEST_F(MouseTest, ViewConstructorCreatesValidView) {
  Mouse::View view{*mouse_};
  EXPECT_EQ(view, *view_);
}

TEST_F(MouseTest, ViewCopyConstructorWorks) {
  Mouse::View view_copy{*view_};
  EXPECT_EQ(view_copy, *view_);
}

TEST_F(MouseTest, ViewMoveConstructorWorks) {
  Mouse::View view_original{*mouse_};
  Mouse::View view_moved{std::move(view_original)};
  EXPECT_EQ(view_moved, *view_);
}

TEST_F(MouseTest, ViewAssignmentOperatorWorks) {
  Mouse second_mouse{Mouse::kDefaultEventsQueueSize};
  Mouse::View second_view{second_mouse};

  second_view = *view_;
  EXPECT_EQ(second_view, *view_);
}

TEST_F(MouseTest, ViewEqualityOperatorWorksCorrectly) {
  Mouse::View same_view{*mouse_};
  Mouse second_mouse{Mouse::kDefaultEventsQueueSize};
  Mouse::View different_view{second_mouse};

  EXPECT_EQ(*view_, same_view);
  EXPECT_NE(*view_, different_view);
}

TEST_F(MouseTest, OnLButtonDownSetsButtonPressed) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnLButtonDown(lParam);

  EXPECT_TRUE(view_->IsLeftButtonPressed());
  EXPECT_FALSE(view_->IsRightButtonPressed());
}

TEST_F(MouseTest, OnLButtonUpClearsButtonPressed) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnLButtonDown(lParam);
  mouse_->OnLButtonUp(lParam);

  EXPECT_FALSE(view_->IsLeftButtonPressed());
}

TEST_F(MouseTest, OnRButtonDownSetsButtonPressed) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnRButtonDown(lParam);

  EXPECT_FALSE(view_->IsLeftButtonPressed());
  EXPECT_TRUE(view_->IsRightButtonPressed());
}

TEST_F(MouseTest, OnRButtonUpClearsButtonPressed) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnRButtonDown(lParam);
  mouse_->OnRButtonUp(lParam);

  EXPECT_FALSE(view_->IsRightButtonPressed());
}

TEST_F(MouseTest, BothButtonsCanBePressedSimultaneously) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnLButtonDown(lParam);
  mouse_->OnRButtonDown(lParam);

  EXPECT_TRUE(view_->IsLeftButtonPressed());
  EXPECT_TRUE(view_->IsRightButtonPressed());
}

TEST_F(MouseTest, OnLButtonDownGeneratesEvent) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnLButtonDown(lParam);

  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kCenterPos.x);
  EXPECT_EQ(event->first.y, kCenterPos.y);
  EXPECT_EQ(event->second, Mouse::EventType::kLeftButtonPressed);
}

TEST_F(MouseTest, OnLButtonUpGeneratesEvent) {
  LPARAM const lParam{MAKELPARAM(kBottomRightPos.x, kBottomRightPos.y)};

  mouse_->OnLButtonUp(lParam);

  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kBottomRightPos.x);
  EXPECT_EQ(event->first.y, kBottomRightPos.y);
  EXPECT_EQ(event->second, Mouse::EventType::kLeftButtonReleased);
}

TEST_F(MouseTest, OnRButtonDownGeneratesEvent) {
  LPARAM const lParam{MAKELPARAM(kOriginPos.x, kOriginPos.y)};

  mouse_->OnRButtonDown(lParam);

  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kOriginPos.x);
  EXPECT_EQ(event->first.y, kOriginPos.y);
  EXPECT_EQ(event->second, Mouse::EventType::kRightButtonPressed);
}

TEST_F(MouseTest, OnRButtonUpGeneratesEvent) {
  LPARAM const lParam{MAKELPARAM(kOriginPos.x, kOriginPos.y)};

  mouse_->OnRButtonUp(lParam);

  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kOriginPos.x);
  EXPECT_EQ(event->first.y, kOriginPos.y);
  EXPECT_EQ(event->second, Mouse::EventType::kRightButtonReleased);
}

TEST_F(MouseTest, OnMoveGeneratesEvent) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnMove(lParam);

  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kCenterPos.x);
  EXPECT_EQ(event->first.y, kCenterPos.y);
  EXPECT_EQ(event->second, Mouse::EventType::kMove);
}

TEST_F(MouseTest, OnHoverWindowGeneratesEventAndSetsInWindow) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnHoverWindow(lParam);

  EXPECT_TRUE(view_->IsInWindow());

  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kCenterPos.x);
  EXPECT_EQ(event->first.y, kCenterPos.y);
  EXPECT_EQ(event->second, Mouse::EventType::kHoverWindow);
}

TEST_F(MouseTest, OnLeaveWindowGeneratesEventAndClearsInWindow) {
  LPARAM const lParam{MAKELPARAM(kNegativePos.x, kNegativePos.y)};

  mouse_->OnHoverWindow(lParam);
  mouse_->OnLeaveWindow(lParam);

  EXPECT_FALSE(view_->IsInWindow());

  view_->GetOldestEvent(); 
  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kNegativePos.x);
  EXPECT_EQ(event->first.y, kNegativePos.y);
  EXPECT_EQ(event->second, Mouse::EventType::kLeaveWindow);
}

TEST_F(MouseTest, OnWheelUpGeneratesCorrectEvents) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};
  WPARAM const wParam{MAKEWPARAM(0, WHEEL_DELTA)};

  mouse_->OnWheel(lParam, wParam);

  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kCenterPos.x);
  EXPECT_EQ(event->first.y, kCenterPos.y);
  EXPECT_EQ(event->second, Mouse::EventType::kWheelUp);
}

TEST_F(MouseTest, OnWheelDownGeneratesCorrectEvents) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};
  WPARAM const wParam{MAKEWPARAM(0, -WHEEL_DELTA)};

  mouse_->OnWheel(lParam, wParam);

  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kCenterPos.x);
  EXPECT_EQ(event->first.y, kCenterPos.y);
  EXPECT_EQ(event->second, Mouse::EventType::kWheelDown);
}

TEST_F(MouseTest, OnWheelAccumulatesDeltaCorrectly) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  WPARAM const half_delta{MAKEWPARAM(0, WHEEL_DELTA / 2)};
  mouse_->OnWheel(lParam, half_delta);
  EXPECT_FALSE(view_->GetOldestEvent().has_value());

  mouse_->OnWheel(lParam, half_delta);
  EXPECT_TRUE(view_->GetOldestEvent().has_value());
}

TEST_F(MouseTest, OnWheelMultipleDeltasGenerateMultipleEvents) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};
  WPARAM const triple_delta{MAKEWPARAM(0, WHEEL_DELTA * 3)};

  mouse_->OnWheel(lParam, triple_delta);

  EXPECT_TRUE(view_->GetOldestEvent().has_value());
  EXPECT_TRUE(view_->GetOldestEvent().has_value());
  EXPECT_TRUE(view_->GetOldestEvent().has_value());
  EXPECT_FALSE(view_->GetOldestEvent().has_value());
}

TEST_F(MouseTest, EventsAreOrderedFIFO) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnLButtonDown(lParam);
  mouse_->OnMove(lParam);
  mouse_->OnLButtonUp(lParam);

  auto const first_event{view_->GetOldestEvent()};
  ASSERT_TRUE(first_event.has_value());
  EXPECT_EQ(first_event->second, Mouse::EventType::kLeftButtonPressed);

  auto const second_event{view_->GetOldestEvent()};
  ASSERT_TRUE(second_event.has_value());
  EXPECT_EQ(second_event->second, Mouse::EventType::kMove);

  auto const third_event{view_->GetOldestEvent()};
  ASSERT_TRUE(third_event.has_value());
  EXPECT_EQ(third_event->second, Mouse::EventType::kLeftButtonReleased);
}

TEST_F(MouseTest, GetOldestEventReturnsNulloptWhenEmpty) {
  EXPECT_FALSE(view_->GetOldestEvent().has_value());
}

TEST_F(MouseTest, NegativeCoordinatesAreHandledCorrectly) {
  LPARAM const lParam{MAKELPARAM(kNegativePos.x, kNegativePos.y)};

  mouse_->OnMove(lParam);

  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kNegativePos.x);
  EXPECT_EQ(event->first.y, kNegativePos.y);
}

TEST_F(MouseTest, LargeCoordinatesAreHandledCorrectly) {
  static constexpr POINTS kLargePos{32767, 32767};
  LPARAM const lParam{MAKELPARAM(kLargePos.x, kLargePos.y)};

  mouse_->OnMove(lParam);

  auto const event{view_->GetOldestEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first.x, kLargePos.x);
  EXPECT_EQ(event->first.y, kLargePos.y);
}

TEST_F(MouseTest, EventQueueHandlesOverflow) {
  static constexpr std::size_t kOverflowCount{
      Mouse::kDefaultEventsQueueSize * 3 / 2};
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  for (std::size_t i{0}; i < kOverflowCount; ++i) {
    mouse_->OnMove(lParam);
  }

  std::size_t event_count{0};
  while (view_->GetOldestEvent().has_value()) {
    ++event_count;
  }

  EXPECT_LE(event_count, Mouse::kDefaultEventsQueueSize);
}

TEST_F(MouseTest, ButtonStateIndependentOfEvents) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnLButtonDown(lParam);
  view_->GetOldestEvent(); 

  EXPECT_TRUE(view_->IsLeftButtonPressed());
  EXPECT_FALSE(view_->GetOldestEvent().has_value());
}

TEST_F(MouseTest, WindowStateIndependentOfEvents) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  mouse_->OnHoverWindow(lParam);
  view_->GetOldestEvent(); 

  EXPECT_TRUE(view_->IsInWindow());
  EXPECT_FALSE(view_->GetOldestEvent().has_value());
}

TEST_F(MouseTest, AllEventTypesHaveUniqueValues) {
  std::set<Mouse::EventType> event_types{Mouse::EventType::kLeftButtonPressed,
                                         Mouse::EventType::kLeftButtonReleased,
                                         Mouse::EventType::kRightButtonPressed,
                                         Mouse::EventType::kRightButtonReleased,
                                         Mouse::EventType::kWheelUp,
                                         Mouse::EventType::kWheelDown,
                                         Mouse::EventType::kMove,
                                         Mouse::EventType::kHoverWindow,
                                         Mouse::EventType::kLeaveWindow};

  EXPECT_EQ(event_types.size(), 9);
}

TEST_F(MouseTest, WheelNegativeAccumulationWorks) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  WPARAM const quarter_delta{MAKEWPARAM(0, -WHEEL_DELTA / 4)};

  mouse_->OnWheel(lParam, quarter_delta);
  mouse_->OnWheel(lParam, quarter_delta);
  mouse_->OnWheel(lParam, quarter_delta);
  EXPECT_FALSE(view_->GetOldestEvent().has_value());

  mouse_->OnWheel(lParam, quarter_delta);
  EXPECT_TRUE(view_->GetOldestEvent().has_value());
}

TEST_F(MouseTest, MixedWheelDirectionsHandledCorrectly) {
  LPARAM const lParam{MAKELPARAM(kCenterPos.x, kCenterPos.y)};

  WPARAM const pos_delta{MAKEWPARAM(0, WHEEL_DELTA / 2)};
  WPARAM const neg_delta{MAKEWPARAM(0, -WHEEL_DELTA / 4)};

  mouse_->OnWheel(lParam, pos_delta);
  mouse_->OnWheel(lParam, neg_delta);

  mouse_->OnWheel(lParam, pos_delta);
  mouse_->OnWheel(lParam, pos_delta);
  EXPECT_TRUE(view_->GetOldestEvent().has_value());
  EXPECT_FALSE(view_->GetOldestEvent().has_value());
}
}  // namespace input_handling
