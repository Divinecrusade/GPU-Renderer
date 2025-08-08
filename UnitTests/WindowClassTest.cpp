#include "pch.h"
#include "WindowClass.hpp"

namespace window_manipulating {
using gpu_renderer::window::WindowClass;

class WindowClassTest : public ::testing::Test {
 protected:
  void SetUp() override {
    hInstance_ = GetModuleHandle(nullptr);
    ASSERT_NE(hInstance_, nullptr) << "Failed to get module handle";

    test_class_name_ = L"TestClass_" + std::to_wstring(GetCurrentProcessId()) +
                       L"_" + std::to_wstring(GetTickCount64());
  }

  void TearDown() override {
    UnregisterClassW(test_class_name_.c_str(), hInstance_);
  }

  static LRESULT CALLBACK TestWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                      LPARAM lParam) {
    return DefWindowProcW(hWnd, message, wParam, lParam);
  }

  HINSTANCE hInstance_{nullptr};
  std::wstring test_class_name_;
};

TEST_F(WindowClassTest, ConstructorRegistersClassSuccessfully) {
  std::unique_ptr<WindowClass> wc{};

  ASSERT_NO_THROW({
    wc = std::make_unique<WindowClass>(
        CS_HREDRAW | CS_VREDRAW, TestWndProc, 0, 0, hInstance_,
        WindowClass::kDefaultIcon, WindowClass::kDefaultCursor,
        WindowClass::kNoBackground, WindowClass::kMenuName,
        test_class_name_.c_str(), WindowClass::kDefaultIcon);
  });

  ASSERT_NE(wc, nullptr);
  EXPECT_STREQ(wc->GetLpClassName(), test_class_name_.c_str());
}

TEST_F(WindowClassTest, GetLpClassnameReturnsCorrectName) {
  WindowClass const wc{CS_HREDRAW | CS_VREDRAW,
                 TestWndProc,
                 0,
                 0,
                 hInstance_,
                 WindowClass::kDefaultIcon,
                 WindowClass::kDefaultCursor,
                 WindowClass::kNoBackground,
                 WindowClass::kMenuName,
                 test_class_name_.c_str(),
                 WindowClass::kDefaultIcon};

  LPCWSTR const class_name{wc.GetLpClassName()};
  EXPECT_STREQ(class_name, test_class_name_.c_str());
  EXPECT_NE(class_name, nullptr);
}

TEST_F(WindowClassTest, MultipleInstancesWithSameNameShareRegistration) {
  std::wstring const shared_class_name{test_class_name_ + L"_shared"};

  std::unique_ptr<WindowClass> first_instance{};
  ASSERT_NO_THROW({
    first_instance = std::make_unique<WindowClass>(
        CS_HREDRAW | CS_VREDRAW, TestWndProc, 0, 0, hInstance_,
        WindowClass::kDefaultIcon, WindowClass::kDefaultCursor,
        WindowClass::kNoBackground, WindowClass::kMenuName,
        shared_class_name.c_str(), WindowClass::kDefaultIcon);
  });

  std::unique_ptr<WindowClass> second_instance{};
  ASSERT_NO_THROW({
    second_instance = std::make_unique<WindowClass>(
        CS_HREDRAW | CS_VREDRAW, TestWndProc, 0, 0, hInstance_,
        WindowClass::kDefaultIcon, WindowClass::kDefaultCursor,
        WindowClass::kNoBackground, WindowClass::kMenuName,
        shared_class_name.c_str(), WindowClass::kDefaultIcon);
  });

  EXPECT_STREQ(first_instance->GetLpClassName(), shared_class_name.c_str());
  EXPECT_STREQ(second_instance->GetLpClassName(), shared_class_name.c_str());

  EXPECT_EQ(first_instance->GetLpClassName(),
            second_instance->GetLpClassName());
}

TEST_F(WindowClassTest, DestructorUnregistersWhenLastInstanceDestroyed) {
  std::wstring const unique_class_name{test_class_name_ + L"_destruct"};

  {
    WindowClass wc{CS_HREDRAW | CS_VREDRAW,
                   TestWndProc,
                   0,
                   0,
                   hInstance_,
                   WindowClass::kDefaultIcon,
                   WindowClass::kDefaultCursor,
                   WindowClass::kNoBackground,
                   WindowClass::kMenuName,
                   unique_class_name.c_str(),
                   WindowClass::kDefaultIcon};

    EXPECT_STREQ(wc.GetLpClassName(), unique_class_name.c_str());
  }  // WindowClass деструктор

  HWND const test_window{CreateWindowExW(0, unique_class_name.c_str(), L"Test",
                                     WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                                     nullptr, nullptr, hInstance_, nullptr)};

  EXPECT_EQ(test_window, nullptr);
  EXPECT_EQ(GetLastError(), ERROR_CANNOT_FIND_WND_CLASS);
}

TEST_F(WindowClassTest, ReferenceCountingWithMultipleInstancesAndDestruction) {
  std::wstring const ref_count_class_name{test_class_name_ + L"_refcount"};

  std::unique_ptr<WindowClass> first_instance{};
  std::unique_ptr<WindowClass> second_instance{};

  ASSERT_NO_THROW({
    first_instance = std::make_unique<WindowClass>(
        CS_HREDRAW | CS_VREDRAW, TestWndProc, 0, 0, hInstance_,
        WindowClass::kDefaultIcon, WindowClass::kDefaultCursor,
        WindowClass::kNoBackground, WindowClass::kMenuName,
        ref_count_class_name.c_str(), WindowClass::kDefaultIcon);
  });

  ASSERT_NO_THROW({
    second_instance = std::make_unique<WindowClass>(
        CS_HREDRAW | CS_VREDRAW, TestWndProc, 0, 0, hInstance_,
        WindowClass::kDefaultIcon, WindowClass::kDefaultCursor,
        WindowClass::kNoBackground, WindowClass::kMenuName,
        ref_count_class_name.c_str(), WindowClass::kDefaultIcon);
  });

  first_instance.reset();

  EXPECT_STREQ(second_instance->GetLpClassName(), ref_count_class_name.c_str());

  HWND test_window{CreateWindowExW(0, ref_count_class_name.c_str(), L"Test",
                                     WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                                     nullptr, nullptr, hInstance_, nullptr)};

  EXPECT_NE(test_window, nullptr);
  if (test_window) {
    DestroyWindow(test_window);
  }

  second_instance.reset();

  test_window = CreateWindowExW(0, ref_count_class_name.c_str(), L"Test",
                                WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr,
                                nullptr, hInstance_, nullptr);

  EXPECT_EQ(test_window, nullptr);
  EXPECT_EQ(GetLastError(), ERROR_CANNOT_FIND_WND_CLASS);
}

TEST_F(WindowClassTest, DifferentClassNamesCreateSeparateRegistrations) {
  std::wstring const first_class_name{test_class_name_ + L"_first"};
  std::wstring const second_class_name{test_class_name_ + L"_second"};

  WindowClass first_class{CS_HREDRAW | CS_VREDRAW,
                          TestWndProc,
                          0,
                          0,
                          hInstance_,
                          WindowClass::kDefaultIcon,
                          WindowClass::kDefaultCursor,
                          WindowClass::kNoBackground,
                          WindowClass::kMenuName,
                          first_class_name.c_str(),
                          WindowClass::kDefaultIcon};

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

  EXPECT_STREQ(first_class.GetLpClassName(), first_class_name.c_str());
  EXPECT_STREQ(second_class.GetLpClassName(), second_class_name.c_str());
  EXPECT_STRNE(first_class.GetLpClassName(), second_class.GetLpClassName());
}

TEST_F(WindowClassTest, WindowCreationWorksWithRegisteredClass) {
  WindowClass const wc{CS_HREDRAW | CS_VREDRAW,
                       TestWndProc,
                       0,
                       0,
                       hInstance_,
                       WindowClass::kDefaultIcon,
                       WindowClass::kDefaultCursor,
                       WindowClass::kNoBackground,
                       WindowClass::kMenuName,
                       test_class_name_.c_str(),
                       WindowClass::kDefaultIcon};

  HWND const test_window{
      CreateWindowExW(0, wc.GetLpClassName(), L"Test Window",
                      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640,
                      480, nullptr, nullptr, hInstance_, nullptr)};

  EXPECT_NE(test_window, nullptr);

  if (test_window) {
    wchar_t retrieved_class_name[256]{L'0'};
    int const result{
        GetClassNameW(test_window, retrieved_class_name,
                      sizeof(retrieved_class_name) / sizeof(wchar_t))};
    EXPECT_GT(result, 0);
    EXPECT_STREQ(retrieved_class_name, test_class_name_.c_str());

    DestroyWindow(test_window);
  }
}

}  // namespace winapi_wrapper