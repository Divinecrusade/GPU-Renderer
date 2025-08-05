#include "pch.h"
#include "Keyboard.hpp"
#include "Window.hpp"
#include "WindowClass.hpp"

namespace input_handling {
using gpu_renderer::Keyboard;
using gpu_renderer::Window;
using gpu_renderer::WindowClass;

class KeyboardTest : public ::testing::Test {
 protected:
  void SetUp() override {
    keyboard_ = std::make_unique<Keyboard>();
    view_ = std::make_unique<Keyboard::View>(*keyboard_);
  }

  void TearDown() override {
    view_.reset();
    keyboard_.reset();
  }

  std::unique_ptr<Keyboard> keyboard_;
  std::unique_ptr<Keyboard::View> view_;

  static constexpr Keyboard::KeyCode kTestKeyA{'A'};
  static constexpr Keyboard::KeyCode kTestKeyB{'B'};
  static constexpr Keyboard::KeyCode kTestKeyEscape{VK_ESCAPE};
  static constexpr wchar_t kTestCharA{L'A'};
  static constexpr wchar_t kTestCharB{L'B'};
  static constexpr wchar_t kTestCharSpace{L' '};
};

TEST_F(KeyboardTest, DefaultConstructorCreatesValidKeyboard) {
  EXPECT_FALSE(keyboard_->IsAutoRepeating());
  EXPECT_FALSE(view_->IsKeyPressed(kTestKeyA));
  EXPECT_FALSE(view_->IsKeyEventHappened());
  EXPECT_FALSE(view_->IsCharTyped());
}

TEST_F(KeyboardTest, ViewConstructorCreatesValidView) {
  Keyboard::View view{*keyboard_};
  EXPECT_EQ(view, *view_);
}

TEST_F(KeyboardTest, ViewCopyConstructorWorks) {
  Keyboard::View view_copy{*view_};
  EXPECT_EQ(view_copy, *view_);
}

TEST_F(KeyboardTest, ViewMoveConstructorWorks) {
  Keyboard::View view_original{*keyboard_};
  Keyboard::View view_moved{std::move(view_original)};
  EXPECT_EQ(view_moved, *view_);
}

TEST_F(KeyboardTest, ViewAssignmentOperatorWorks) {
  Keyboard second_keyboard{};
  Keyboard::View second_view{second_keyboard};

  second_view = *view_;
  EXPECT_EQ(second_view, *view_);
}

TEST_F(KeyboardTest, ViewEqualityOperatorWorksCorrectly) {
  Keyboard::View same_view{*keyboard_};
  Keyboard second_keyboard{};
  Keyboard::View different_view{second_keyboard};

  EXPECT_EQ(*view_, same_view);
  EXPECT_NE(*view_, different_view);
}

TEST_F(KeyboardTest, AutoRepeatDefaultsToDisabled) {
  EXPECT_FALSE(keyboard_->IsAutoRepeating());
}

TEST_F(KeyboardTest, EnableAutoRepeatChangesState) {
  keyboard_->EnableAutoRepeat();
  EXPECT_TRUE(keyboard_->IsAutoRepeating());
}

TEST_F(KeyboardTest, DisableAutoRepeatChangesState) {
  keyboard_->EnableAutoRepeat();
  keyboard_->DisableAutoRepeat();
  EXPECT_FALSE(keyboard_->IsAutoRepeating());
}

TEST_F(KeyboardTest, MultipleEnableAutoRepeatCallsAreIdempotent) {
  keyboard_->EnableAutoRepeat();
  keyboard_->EnableAutoRepeat();
  EXPECT_TRUE(keyboard_->IsAutoRepeating());
}

TEST_F(KeyboardTest, MultipleDisableAutoRepeatCallsAreIdempotent) {
  keyboard_->EnableAutoRepeat();
  keyboard_->DisableAutoRepeat();
  keyboard_->DisableAutoRepeat();
  EXPECT_FALSE(keyboard_->IsAutoRepeating());
}

TEST_F(KeyboardTest, OnKeyDownSetsKeyPressed) {
  keyboard_->OnKeyDown(kTestKeyA);
  EXPECT_TRUE(view_->IsKeyPressed(kTestKeyA));
}

TEST_F(KeyboardTest, OnKeyUpClearsKeyPressed) {
  keyboard_->OnKeyDown(kTestKeyA);
  keyboard_->OnKeyUp(kTestKeyA);
  EXPECT_FALSE(view_->IsKeyPressed(kTestKeyA));
}

TEST_F(KeyboardTest, MultipleKeysCanBePressedSimultaneously) {
  keyboard_->OnKeyDown(kTestKeyA);
  keyboard_->OnKeyDown(kTestKeyB);

  EXPECT_TRUE(view_->IsKeyPressed(kTestKeyA));
  EXPECT_TRUE(view_->IsKeyPressed(kTestKeyB));
}

TEST_F(KeyboardTest, OnKeyUpOnlyAffectsSpecificKey) {
  keyboard_->OnKeyDown(kTestKeyA);
  keyboard_->OnKeyDown(kTestKeyB);
  keyboard_->OnKeyUp(kTestKeyA);

  EXPECT_FALSE(view_->IsKeyPressed(kTestKeyA));
  EXPECT_TRUE(view_->IsKeyPressed(kTestKeyB));
}

TEST_F(KeyboardTest, ClearKeysStateClearsAllKeys) {
  keyboard_->OnKeyDown(kTestKeyA);
  keyboard_->OnKeyDown(kTestKeyB);
  keyboard_->OnKeyDown(kTestKeyEscape);

  keyboard_->ClearKeysState();

  EXPECT_FALSE(view_->IsKeyPressed(kTestKeyA));
  EXPECT_FALSE(view_->IsKeyPressed(kTestKeyB));
  EXPECT_FALSE(view_->IsKeyPressed(kTestKeyEscape));
}

TEST_F(KeyboardTest, OnKeyDownGeneratesKeyEvent) {
  keyboard_->OnKeyDown(kTestKeyA);

  EXPECT_TRUE(view_->IsKeyEventHappened());

  auto const event{view_->GetOldestKeyEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first, kTestKeyA);
  EXPECT_EQ(event->second, Keyboard::EventType::kPressed);
}

TEST_F(KeyboardTest, OnKeyUpGeneratesKeyEvent) {
  keyboard_->OnKeyUp(kTestKeyA);

  EXPECT_TRUE(view_->IsKeyEventHappened());

  auto const event{view_->GetOldestKeyEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->first, kTestKeyA);
  EXPECT_EQ(event->second, Keyboard::EventType::kReleased);
}

TEST_F(KeyboardTest, KeyEventsAreOrderedFIFO) {
  keyboard_->OnKeyDown(kTestKeyA);
  keyboard_->OnKeyUp(kTestKeyA);
  keyboard_->OnKeyDown(kTestKeyB);

  auto const first_event{view_->GetOldestKeyEvent()};
  ASSERT_TRUE(first_event.has_value());
  EXPECT_EQ(first_event->first, kTestKeyA);
  EXPECT_EQ(first_event->second, Keyboard::EventType::kPressed);

  auto const second_event{view_->GetOldestKeyEvent()};
  ASSERT_TRUE(second_event.has_value());
  EXPECT_EQ(second_event->first, kTestKeyA);
  EXPECT_EQ(second_event->second, Keyboard::EventType::kReleased);

  auto const third_event{view_->GetOldestKeyEvent()};
  ASSERT_TRUE(third_event.has_value());
  EXPECT_EQ(third_event->first, kTestKeyB);
  EXPECT_EQ(third_event->second, Keyboard::EventType::kPressed);
}

TEST_F(KeyboardTest, GetOldestKeyEventReturnsNulloptWhenEmpty) {
  auto const event{view_->GetOldestKeyEvent()};
  EXPECT_FALSE(event.has_value());
}

TEST_F(KeyboardTest, IsKeyEventHappenedReturnsFalseWhenEmpty) {
  EXPECT_FALSE(view_->IsKeyEventHappened());
}

TEST_F(KeyboardTest, ClearKeyEventsQueueRemovesAllEvents) {
  keyboard_->OnKeyDown(kTestKeyA);
  keyboard_->OnKeyDown(kTestKeyB);

  keyboard_->ClearKeyEventsQueue();

  EXPECT_FALSE(view_->IsKeyEventHappened());
  EXPECT_FALSE(view_->GetOldestKeyEvent().has_value());
}

TEST_F(KeyboardTest, OnCharAddsCharacterToBuffer) {
  keyboard_->OnChar(kTestCharA);

  EXPECT_TRUE(view_->IsCharTyped());

  auto const character{view_->GetFirstChar()};
  ASSERT_TRUE(character.has_value());
  EXPECT_EQ(character.value(), kTestCharA);
}

TEST_F(KeyboardTest, MultipleCharsAreOrderedFIFO) {
  keyboard_->OnChar(kTestCharA);
  keyboard_->OnChar(kTestCharB);
  keyboard_->OnChar(kTestCharSpace);

  auto const first_char{view_->GetFirstChar()};
  ASSERT_TRUE(first_char.has_value());
  EXPECT_EQ(first_char.value(), kTestCharA);

  auto const second_char{view_->GetFirstChar()};
  ASSERT_TRUE(second_char.has_value());
  EXPECT_EQ(second_char.value(), kTestCharB);

  auto const third_char{view_->GetFirstChar()};
  ASSERT_TRUE(third_char.has_value());
  EXPECT_EQ(third_char.value(), kTestCharSpace);
}

TEST_F(KeyboardTest, GetFirstCharReturnsNulloptWhenEmpty) {
  auto const character{view_->GetFirstChar()};
  EXPECT_FALSE(character.has_value());
}

TEST_F(KeyboardTest, IsCharTypedReturnsFalseWhenEmpty) {
  EXPECT_FALSE(view_->IsCharTyped());
}

TEST_F(KeyboardTest, GetCharsReturnsAllCharactersAsString) {
  keyboard_->OnChar(kTestCharA);
  keyboard_->OnChar(kTestCharB);
  keyboard_->OnChar(kTestCharSpace);

  std::wstring const chars{view_->GetChars()};
  std::wstring const expected{L"AB "};

  EXPECT_EQ(chars, expected);
  EXPECT_FALSE(view_->IsCharTyped());
}

TEST_F(KeyboardTest, GetCharsReturnsEmptyStringWhenEmpty) {
  std::wstring const chars{view_->GetChars()};
  EXPECT_TRUE(chars.empty());
}

TEST_F(KeyboardTest, ClearCharsBufferRemovesAllCharacters) {
  keyboard_->OnChar(kTestCharA);
  keyboard_->OnChar(kTestCharB);

  keyboard_->ClearCharsBuffer();

  EXPECT_FALSE(view_->IsCharTyped());
  EXPECT_FALSE(view_->GetFirstChar().has_value());
  EXPECT_TRUE(view_->GetChars().empty());
}

TEST_F(KeyboardTest, KeyEventsAndCharsAreIndependent) {
  keyboard_->OnKeyDown(kTestKeyA);
  keyboard_->OnChar(kTestCharA);

  EXPECT_TRUE(view_->IsKeyEventHappened());
  EXPECT_TRUE(view_->IsCharTyped());
  EXPECT_TRUE(view_->IsKeyPressed(kTestKeyA));
}

TEST_F(KeyboardTest, ClearOperationsAreIndependent) {
  keyboard_->OnKeyDown(kTestKeyA);
  keyboard_->OnChar(kTestCharA);

  keyboard_->ClearKeysState();

  EXPECT_FALSE(view_->IsKeyPressed(kTestKeyA));
  EXPECT_TRUE(view_->IsKeyEventHappened());
  EXPECT_TRUE(view_->IsCharTyped());

  keyboard_->ClearKeyEventsQueue();

  EXPECT_FALSE(view_->IsKeyEventHappened());
  EXPECT_TRUE(view_->IsCharTyped());

  keyboard_->ClearCharsBuffer();

  EXPECT_FALSE(view_->IsCharTyped());
}

TEST_F(KeyboardTest, SameKeyPressedMultipleTimesGeneratesMultipleEvents) {
  keyboard_->OnKeyDown(kTestKeyA);
  keyboard_->OnKeyDown(kTestKeyA);
  keyboard_->OnKeyDown(kTestKeyA);

  auto const first_event{view_->GetOldestKeyEvent()};
  auto const second_event{view_->GetOldestKeyEvent()};
  auto const third_event{view_->GetOldestKeyEvent()};

  EXPECT_TRUE(first_event.has_value());
  EXPECT_TRUE(second_event.has_value());
  EXPECT_TRUE(third_event.has_value());

  EXPECT_TRUE(view_->IsKeyPressed(kTestKeyA));
}

TEST_F(KeyboardTest, KeyUpWithoutKeyDownGeneratesEvent) {
  keyboard_->OnKeyUp(kTestKeyA);

  EXPECT_TRUE(view_->IsKeyEventHappened());
  EXPECT_FALSE(view_->IsKeyPressed(kTestKeyA));

  auto const event{view_->GetOldestKeyEvent()};
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ(event->second, Keyboard::EventType::kReleased);
}

TEST_F(KeyboardTest, UnicodeCharactersAreHandledCorrectly) {
  static constexpr wchar_t kUnicodeChar{L'ñ'};
  static constexpr wchar_t kEmojiChar{L'😀'};

  keyboard_->OnChar(kUnicodeChar);
  keyboard_->OnChar(kEmojiChar);

  auto const first_char{view_->GetFirstChar()};
  auto const second_char{view_->GetFirstChar()};

  ASSERT_TRUE(first_char.has_value());
  ASSERT_TRUE(second_char.has_value());

  EXPECT_EQ(first_char.value(), kUnicodeChar);
  EXPECT_EQ(second_char.value(), kEmojiChar);
}

TEST_F(KeyboardTest, ExtremeBoundaryKeyCodesWork) {
  static constexpr Keyboard::KeyCode kMinKeyCode{0};
  static constexpr Keyboard::KeyCode kMaxKeyCode{255};

  keyboard_->OnKeyDown(kMinKeyCode);
  keyboard_->OnKeyDown(kMaxKeyCode);

  EXPECT_TRUE(view_->IsKeyPressed(kMinKeyCode));
  EXPECT_TRUE(view_->IsKeyPressed(kMaxKeyCode));
}

TEST_F(KeyboardTest, KeyEventQueueHandlesOverflow) {
  static constexpr std::size_t kOverflowCount{300};

  for (std::size_t i{0}; i < kOverflowCount; ++i) {
    keyboard_->OnKeyDown(static_cast<Keyboard::KeyCode>(i % 256));
  }

  EXPECT_TRUE(view_->IsKeyEventHappened());

  std::size_t event_count{0};
  while (view_->GetOldestKeyEvent().has_value()) {
    ++event_count;
  }

  EXPECT_LE(event_count, 256);
}

TEST_F(KeyboardTest, CharBufferHandlesOverflow) {
  static constexpr std::size_t kOverflowCount{300};

  for (wchar_t i{0}; i < kOverflowCount; ++i) {
    keyboard_->OnChar(static_cast<wchar_t>(L'A' + (i % 26)));
  }

  std::wstring const chars{view_->GetChars()};

  EXPECT_LE(chars.size(), 256);
}

}  // namespace input_handling