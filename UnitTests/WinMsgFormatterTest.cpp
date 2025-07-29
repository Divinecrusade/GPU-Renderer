#include "pch.h"

#include "WinMsgFormatter.hpp"

namespace debug_loggin {
class WinMsgFormatterTest : public ::testing::Test {
 protected:
  WinMsgFormatter formatter;
};

TEST_F(WinMsgFormatterTest, LeftButtonDownAtOrigin) {
  static constexpr POINT LEFT_TOP{0, 0};
  UINT const msg{WM_LBUTTONDOWN};
  WPARAM const wParam{MK_LBUTTON};
  LPARAM const lParam{MAKELPARAM(LEFT_TOP.x, LEFT_TOP.y)};

  std::wstring const result{formatter(msg, wParam, lParam)};

  EXPECT_NE(result.find(L"WM_LBUTTONDOWN"), std::wstring::npos);
  EXPECT_NE(result.find(L"LP: 0x0000000000000000"), std::wstring::npos);
  EXPECT_NE(result.find(L"WP: 0x0000000000000001"), std::wstring::npos);
}

TEST_F(WinMsgFormatterTest, LeftButtonDownAtBottomRight) {
  static constexpr POINT RIGHT_BOTTOM{640, 480};
  UINT const msg{WM_LBUTTONDOWN};
  WPARAM const wParam{MK_LBUTTON};
  LPARAM const lParam{MAKELPARAM(RIGHT_BOTTOM.x, RIGHT_BOTTOM.y)};

  std::wstring const result{formatter(msg, wParam, lParam)};

  EXPECT_NE(result.find(L"WM_LBUTTONDOWN"), std::wstring::npos);
  // 640 = 0x280, 480 = 0x1E0, combined: 0x01E00280
  EXPECT_NE(result.find(L"LP: 0x0000000001E00280"), std::wstring::npos);
  EXPECT_NE(result.find(L"WP: 0x0000000000000001"), std::wstring::npos);
}

TEST_F(WinMsgFormatterTest, KeyPressedWithShift) {
  UINT const msg{WM_KEYDOWN};
  WPARAM const wParam{'T'};
  LPARAM const lParam{0x00140001};

  std::wstring const result{formatter(msg, wParam, lParam)};

  EXPECT_NE(result.find(L"WM_KEYDOWN"), std::wstring::npos);
  // 'T' key code = 0x54
  EXPECT_NE(result.find(L"WP: 0x0000000000000054"), std::wstring::npos);
  EXPECT_NE(result.find(L"LP: 0x0000000000140001"), std::wstring::npos);
}

TEST_F(WinMsgFormatterTest, KeyPressedWithoutShift) {
  UINT const msg{WM_KEYDOWN};
  WPARAM const wParam{'W'};
  LPARAM const lParam{0x00110001};

  std::wstring const result{formatter(msg, wParam, lParam)};

  EXPECT_NE(result.find(L"WM_KEYDOWN"), std::wstring::npos);
  // 'W' key code = 0x57
  EXPECT_NE(result.find(L"WP: 0x0000000000000057"), std::wstring::npos);
  EXPECT_NE(result.find(L"LP: 0x0000000000110001"), std::wstring::npos);
}

TEST_F(WinMsgFormatterTest, SystemKeyPressed) {
  UINT const msg{WM_SYSKEYDOWN};
  WPARAM const wParam{VK_ESCAPE};
  LPARAM const lParam{0x20010001};

  std::wstring const result{formatter(msg, wParam, lParam)};

  EXPECT_NE(result.find(L"WM_SYSKEYDOWN"), std::wstring::npos);
  // ESC key code = 0x1B
  EXPECT_NE(result.find(L"WP: 0x000000000000001B"), std::wstring::npos);
  EXPECT_NE(result.find(L"LP: 0x0000000020010001"), std::wstring::npos);
}
TEST_F(WinMsgFormatterTest, UnknownMessage) {
  UINT const msg{0x9999};
  WPARAM const wParam{0x1234};
  LPARAM const lParam{0x5678};

  std::wstring const result{formatter(msg, wParam, lParam)};

  EXPECT_NE(result.find(L"UNKNOWN"), std::wstring::npos);
  EXPECT_NE(result.find(L"0x00009999"), std::wstring::npos);
  EXPECT_NE(result.find(L"WP: 0x0000000000001234"), std::wstring::npos);
  EXPECT_NE(result.find(L"LP: 0x0000000000005678"), std::wstring::npos);
}

TEST_F(WinMsgFormatterTest, FormattingConsistency) {
  std::wstring const result{formatter(WM_CREATE, 0, 0)};

  EXPECT_NE(result.find(L"Msg: "), std::wstring::npos);
  EXPECT_NE(result.find(L"LP: 0x"), std::wstring::npos);
  EXPECT_NE(result.find(L"WP: 0x"), std::wstring::npos);
  EXPECT_NE(result.find(L"WM_CREATE"), std::wstring::npos);
}
}  // namespace debug_loggin