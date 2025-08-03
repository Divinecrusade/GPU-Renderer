#include "pch.h"
#include "WindowClass.hpp"
#include "Window.hpp"
#include "WinError.hpp"

namespace winapi_wrapper {
using gpu_renderer::Window;
using gpu_renderer::WindowClass;
using gpu_renderer::exception::WinError;

class WindowTest : public ::testing::Test {
 protected:
  void SetUp() override {
    hInstance_ = GetModuleHandle(nullptr);
    ASSERT_NE(hInstance_, nullptr) << "Failed to get module handle";

    test_class_name_ = L"WindowTestClass_" +
                       std::to_wstring(GetCurrentProcessId()) + L"_" +
                       std::to_wstring(GetTickCount64());

    test_window_class_ = std::make_unique<WindowClass>(
        CS_HREDRAW | CS_VREDRAW, TestWndProc, 0, 0, hInstance_,
        WindowClass::kDefaultIcon, WindowClass::kDefaultCursor,
        WindowClass::kNoBackground, WindowClass::kMenuName,
        test_class_name_.c_str(), WindowClass::kDefaultIcon);
  }

  void TearDown() override {
    test_window_class_.reset();
    UnregisterClassW(test_class_name_.c_str(), hInstance_);
  }

  static LRESULT CALLBACK TestWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                      LPARAM lParam) {
    return DefWindowProcW(hWnd, message, wParam, lParam);
  }

  HINSTANCE hInstance_{nullptr};
  std::wstring test_class_name_;
  std::unique_ptr<WindowClass> test_window_class_;

  static constexpr LPCWSTR kTestWindowName{L"Test Window"};
  static constexpr int kTestX{100};
  static constexpr int kTestY{100};
  static constexpr int kTestWidth{640};
  static constexpr int kTestHeight{480};
  static constexpr DWORD kTestStyle{WS_OVERLAPPEDWINDOW};
  static constexpr DWORD kTestExStyle{WS_EX_CLIENTEDGE};
};

TEST_F(WindowTest, ConstructorCreatesWindowSuccessfully) {
  std::unique_ptr<Window> window{};

  ASSERT_NO_THROW({
    window = std::make_unique<Window>(*test_window_class_, kTestWindowName,
                                      kTestStyle, kTestX, kTestY, kTestWidth,
                                      kTestHeight, hInstance_, kTestExStyle);
  });

  ASSERT_NE(window, nullptr);
}

TEST_F(WindowTest, ConstructorWithMinimalValidParameters) {
  ASSERT_NO_THROW({
    Window window(*test_window_class_, L"Minimal", WS_POPUP, 0, 0, 1, 1,
                  hInstance_, Window::kNoExtraStyle);
  });
}

TEST_F(WindowTest, ShowWithoutParameterAfterInitialShow) {
  Window window{
      *test_window_class_, kTestWindowName, kTestStyle, kTestX,      kTestY,
      kTestWidth,          kTestHeight,     hInstance_, kTestExStyle};

  window.Show(SW_HIDE);
  ASSERT_NO_THROW(window.Show());
}

TEST_F(WindowTest, HideAfterShow) {
  Window window{
      *test_window_class_, kTestWindowName, kTestStyle, kTestX,      kTestY,
      kTestWidth,          kTestHeight,     hInstance_, kTestExStyle};

  window.Show(SW_SHOW);
  ASSERT_NO_THROW(window.Hide());
}

TEST_F(WindowTest, MultipleShowHideCycles) {
  Window window{
      *test_window_class_, kTestWindowName, kTestStyle, kTestX,      kTestY,
      kTestWidth,          kTestHeight,     hInstance_, kTestExStyle};

  window.Show(SW_SHOW);

  for (int i = 0; i < 3; ++i) {
    ASSERT_NO_THROW(window.Hide());
    ASSERT_NO_THROW(window.Show());
  }
}

TEST_F(WindowTest, GetWindowProcedureReturnsValidPointer) {
  WNDPROC proc = Window::GetlpfnWndProc();
  EXPECT_NE(proc, nullptr);
}

TEST_F(WindowTest, WindowProcedureConsistencyAcrossCalls) {
  WNDPROC proc1 = Window::GetlpfnWndProc();
  WNDPROC proc2 = Window::GetlpfnWndProc();

  EXPECT_EQ(proc1, proc2);
}

class TestableWindow : public Window {
 public:
  TestableWindow(WindowClass const& window_class, LPCWSTR lpszWindowName,
                 DWORD dwStyle, int x, int y, int nWidth, int nHeight,
                 HINSTANCE hInstance, DWORD dwExStyle)
      : Window(window_class, lpszWindowName, dwStyle, x, y, nWidth, nHeight,
               hInstance, dwExStyle) {}

  LRESULT TestHandleWinMessage(UINT Msg, WPARAM wParam, LPARAM lParam) {
    return HandleMessage(Msg, wParam, lParam);
  }

  bool close_message_received_{false};

 protected:
  LRESULT HandleMessage(UINT Msg, WPARAM wParam,
                           LPARAM lParam) noexcept override {
    if (Msg == WM_CLOSE) {
      close_message_received_ = true;
      // Don't call PostQuitMessage in tests
      return 0;
    }
    return Window::HandleMessage(Msg, wParam, lParam);
  }
};

TEST_F(WindowTest, HandleWinMessageProcessesCloseMessage) {
  TestableWindow window{
      *test_window_class_, kTestWindowName, kTestStyle, kTestX,      kTestY,
      kTestWidth,          kTestHeight,     hInstance_, kTestExStyle};

  LRESULT result = window.TestHandleWinMessage(WM_CLOSE, 0, 0);

  EXPECT_EQ(result, 0);
  EXPECT_TRUE(window.close_message_received_);
}

TEST_F(WindowTest, HandleWinMessageForwardsUnknownMessages) {
  TestableWindow window{
      *test_window_class_, kTestWindowName, kTestStyle, kTestX,      kTestY,
      kTestWidth,          kTestHeight,     hInstance_, kTestExStyle};

  LRESULT result = window.TestHandleWinMessage(WM_GETTEXT, 0, 0);

  EXPECT_FALSE(window.close_message_received_);
}

TEST_F(WindowTest, WindowCreationWithDifferentStyles) {
  struct StyleTest {
    DWORD style;
    DWORD ex_style;
    LPCWSTR description;
  };

  std::vector<StyleTest> style_tests{
      {WS_POPUP, 0, L"Popup window"},
      {WS_OVERLAPPED, WS_EX_TOPMOST, L"Overlapped with topmost"},
      {WS_VISIBLE, WS_EX_TOOLWINDOW, L"Visible with tool window"}};

  for (auto const& test : style_tests) {
    ASSERT_NO_THROW({
      Window window(*test_window_class_, test.description, test.style, kTestX,
                    kTestY, kTestWidth, kTestHeight, hInstance_, test.ex_style);
    }) << "Failed to create window with style: "
       << test.description;
  }
}

TEST_F(WindowTest, MultipleWindowInstancesCoexist) {
  std::vector<std::unique_ptr<Window>> windows;

  for (int i = 0; i < 3; ++i) {
    std::wstring window_name = std::wstring{L"Window "} + std::to_wstring(i);

    ASSERT_NO_THROW({
      windows.emplace_back(std::make_unique<Window>(
          *test_window_class_, window_name.c_str(), kTestStyle, kTestX + i * 50,
          kTestY + i * 50, kTestWidth, kTestHeight, hInstance_, kTestExStyle));
    });
  }

  EXPECT_EQ(windows.size(), 3);

  for (auto& window : windows) {
    ASSERT_NO_THROW(window->Show(SW_HIDE));
  }
}

TEST_F(WindowTest, WindowWithDifferentWindowClasses) {
  std::wstring second_class_name = test_class_name_ + L"_Second";
  WindowClass second_class{CS_HREDRAW | CS_VREDRAW,
                           TestWndProc,
                           0,
                           0,
                           hInstance_,
                           WindowClass::kDefaultIcon,
                           WindowClass::kDefaultCursor,
                           WindowClass::kNoBackground,
                           WindowClass::kMenuName,
                           second_class_name.c_str(),
                           WindowClass::kDefaultIcon};

  Window first_window{
      *test_window_class_, L"First Window", kTestStyle, kTestX,      kTestY,
      kTestWidth,          kTestHeight,     hInstance_, kTestExStyle};

  Window second_window{second_class, L"Second Window", kTestStyle,
                       kTestX + 100, kTestY + 100,     kTestWidth,
                       kTestHeight,  hInstance_,       kTestExStyle};

  ASSERT_NO_THROW(first_window.Show(SW_HIDE));
  ASSERT_NO_THROW(second_window.Show(SW_HIDE));
}

TEST_F(WindowTest, WindowDimensionBoundaryValues) {
  struct DimensionTest {
    int width;
    int height;
    bool should_succeed;
  };

  std::vector<DimensionTest> dimension_tests{
      {1, 1, true},        
      {32767, 32767, true}, 
      {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
       true} 
  };

  for (auto const& test : dimension_tests) {
    if (test.should_succeed) {
      ASSERT_NO_THROW({
        Window window(*test_window_class_, L"Dimension Test", kTestStyle, 0, 0,
                      test.width, test.height, hInstance_, kTestExStyle);
      }) << "Failed with dimensions: "
         << test.width << "x" << test.height;
    }
  }
}
}  // namespace winapi_wrapper