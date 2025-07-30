#ifndef WIN_MSG_FORMATTER_HPP
#define WIN_MSG_FORMATTER_HPP
#include <WinUser.h>

#include <iomanip>
#include <sstream>
#include <string>

namespace gpu_renderer {
struct WinMsgFormatter {
 private:
  static constexpr std::wstring_view kUnknownMessageName{L"UNKNOWN"};
  static constexpr std::wstring_view kHexLiteral{L"0x"};
  static constexpr auto kHexDigitsInByte{2};
  static constexpr std::streamsize kMessageNameWidth{24};
  static constexpr std::streamsize kMessageCodeWidth{
      static_cast<std::streamsize>(sizeof(UINT)) * kHexDigitsInByte};
  static constexpr std::streamsize kMessageWparamWidth{
      static_cast<std::streamsize>(sizeof(WPARAM)) * kHexDigitsInByte};
  static constexpr std::streamsize kMessageLparamWidth{
      static_cast<std::streamsize>(sizeof(LPARAM)) * kHexDigitsInByte};

 public:
  std::wstring operator()(UINT Msg, WPARAM wParam, LPARAM lParam) {
    std::wostringstream wsout{};
    wsout << L"Msg: ";
    wsout << std::setw(kMessageNameWidth) << std::right;

    switch (Msg) {
#define KNOWN_MESSAGE_NAME(Msg) \
  case Msg:                     \
    wsout << L#Msg;             \
    break;
      // Window management messages
      KNOWN_MESSAGE_NAME(WM_CREATE)
      KNOWN_MESSAGE_NAME(WM_DESTROY)
      KNOWN_MESSAGE_NAME(WM_MOVE)
      KNOWN_MESSAGE_NAME(WM_SIZE)
      KNOWN_MESSAGE_NAME(WM_ACTIVATE)
      KNOWN_MESSAGE_NAME(WM_SETFOCUS)
      KNOWN_MESSAGE_NAME(WM_KILLFOCUS)
      KNOWN_MESSAGE_NAME(WM_ENABLE)
      KNOWN_MESSAGE_NAME(WM_SETREDRAW)
      KNOWN_MESSAGE_NAME(WM_SETTEXT)
      KNOWN_MESSAGE_NAME(WM_GETTEXT)
      KNOWN_MESSAGE_NAME(WM_GETTEXTLENGTH)
      KNOWN_MESSAGE_NAME(WM_PAINT)
      KNOWN_MESSAGE_NAME(WM_CLOSE)
      KNOWN_MESSAGE_NAME(WM_QUIT)
      KNOWN_MESSAGE_NAME(WM_ERASEBKGND)
      KNOWN_MESSAGE_NAME(WM_SYSCOLORCHANGE)
      KNOWN_MESSAGE_NAME(WM_SHOWWINDOW)
      KNOWN_MESSAGE_NAME(WM_SETTINGCHANGE)

      // Input messages
      KNOWN_MESSAGE_NAME(WM_KEYDOWN)
      KNOWN_MESSAGE_NAME(WM_KEYUP)
      KNOWN_MESSAGE_NAME(WM_CHAR)
      KNOWN_MESSAGE_NAME(WM_DEADCHAR)
      KNOWN_MESSAGE_NAME(WM_SYSKEYDOWN)
      KNOWN_MESSAGE_NAME(WM_SYSKEYUP)
      KNOWN_MESSAGE_NAME(WM_SYSCHAR)
      KNOWN_MESSAGE_NAME(WM_SYSDEADCHAR)

      // Mouse messages
      KNOWN_MESSAGE_NAME(WM_MOUSEMOVE)
      KNOWN_MESSAGE_NAME(WM_LBUTTONDOWN)
      KNOWN_MESSAGE_NAME(WM_LBUTTONUP)
      KNOWN_MESSAGE_NAME(WM_LBUTTONDBLCLK)
      KNOWN_MESSAGE_NAME(WM_RBUTTONDOWN)
      KNOWN_MESSAGE_NAME(WM_RBUTTONUP)
      KNOWN_MESSAGE_NAME(WM_RBUTTONDBLCLK)
      KNOWN_MESSAGE_NAME(WM_MBUTTONDOWN)
      KNOWN_MESSAGE_NAME(WM_MBUTTONUP)
      KNOWN_MESSAGE_NAME(WM_MBUTTONDBLCLK)
      KNOWN_MESSAGE_NAME(WM_MOUSEWHEEL)
      KNOWN_MESSAGE_NAME(WM_MOUSEHWHEEL)

      // System messages
      KNOWN_MESSAGE_NAME(WM_COMMAND)
      KNOWN_MESSAGE_NAME(WM_SYSCOMMAND)
      KNOWN_MESSAGE_NAME(WM_TIMER)
      KNOWN_MESSAGE_NAME(WM_HSCROLL)
      KNOWN_MESSAGE_NAME(WM_VSCROLL)
      KNOWN_MESSAGE_NAME(WM_INITMENU)
      KNOWN_MESSAGE_NAME(WM_INITMENUPOPUP)
      KNOWN_MESSAGE_NAME(WM_MENUSELECT)
      KNOWN_MESSAGE_NAME(WM_MENUCHAR)
      KNOWN_MESSAGE_NAME(WM_ENTERIDLE)

      // Clipboard messages
      KNOWN_MESSAGE_NAME(WM_CUT)
      KNOWN_MESSAGE_NAME(WM_COPY)
      KNOWN_MESSAGE_NAME(WM_PASTE)
      KNOWN_MESSAGE_NAME(WM_CLEAR)
      KNOWN_MESSAGE_NAME(WM_UNDO)

      // Non-client area messages
      KNOWN_MESSAGE_NAME(WM_NCCREATE)
      KNOWN_MESSAGE_NAME(WM_NCDESTROY)
      KNOWN_MESSAGE_NAME(WM_NCCALCSIZE)
      KNOWN_MESSAGE_NAME(WM_NCHITTEST)
      KNOWN_MESSAGE_NAME(WM_NCPAINT)
      KNOWN_MESSAGE_NAME(WM_NCACTIVATE)
      KNOWN_MESSAGE_NAME(WM_NCMOUSEMOVE)
      KNOWN_MESSAGE_NAME(WM_NCLBUTTONDOWN)
      KNOWN_MESSAGE_NAME(WM_NCLBUTTONUP)
      KNOWN_MESSAGE_NAME(WM_NCLBUTTONDBLCLK)
      KNOWN_MESSAGE_NAME(WM_NCRBUTTONDOWN)
      KNOWN_MESSAGE_NAME(WM_NCRBUTTONUP)
      KNOWN_MESSAGE_NAME(WM_NCRBUTTONDBLCLK)

      // Other common messages
      KNOWN_MESSAGE_NAME(WM_GETMINMAXINFO)
      KNOWN_MESSAGE_NAME(WM_WINDOWPOSCHANGING)
      KNOWN_MESSAGE_NAME(WM_WINDOWPOSCHANGED)
      KNOWN_MESSAGE_NAME(WM_NOTIFY)
      KNOWN_MESSAGE_NAME(WM_CONTEXTMENU)
      KNOWN_MESSAGE_NAME(WM_STYLECHANGING)
      KNOWN_MESSAGE_NAME(WM_STYLECHANGED)
      KNOWN_MESSAGE_NAME(WM_DISPLAYCHANGE)
      KNOWN_MESSAGE_NAME(WM_GETICON)
      KNOWN_MESSAGE_NAME(WM_SETICON)
      KNOWN_MESSAGE_NAME(WM_CAPTURECHANGED)
      KNOWN_MESSAGE_NAME(WM_DEVICECHANGE)
      KNOWN_MESSAGE_NAME(WM_POWERBROADCAST)
#undef KNOWN_MESSAGE_NAME
      default:
        wsout << kUnknownMessageName;
        break;
    }
    wsout << L" " << kHexLiteral << std::uppercase << std::hex
          << std::setfill(L'0') << std::setw(kMessageCodeWidth) << Msg;
    wsout << L" LP: " << kHexLiteral << std::uppercase << std::hex
          << std::setfill(L'0') << std::setw(kMessageWparamWidth) << lParam;
    wsout << L" WP: " << kHexLiteral << std::uppercase << std::hex
          << std::setfill(L'0') << std::setw(kMessageLparamWidth) << wParam;
    return wsout.str();
  }
};
}  // namespace gpu_renderer

#endif  // !WIN_MSG_FORMATTER_HPP