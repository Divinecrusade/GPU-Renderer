#include "WindowClass.hpp"
#include "DebugHeader.hpp"

#ifdef DCONSOLE
#include <iostream>
#endif // DCONSOLE

gpu_renderer::WindowClass::WindowClass(
    UINT style, WNDPROC lpfnWndProc, int cbClsExtra, int cbWndExtra,
    HINSTANCE hInstance, HICON hIcon, HCURSOR hCursor, HBRUSH hbrBackground,
    LPCWSTR lpszMenuName, LPCWSTR lpszClassName, HICON hIconSm) noexcept
    : hInstance{hInstance}, lpClassName{lpszClassName} {
  static constexpr auto ClassRegistrationFailed = [](ATOM aClass) {
    return aClass == NULL;
  };
  WNDCLASSEXW const wc{.cbSize = sizeof(WNDCLASSEX),
                       .style = style,
                       .lpfnWndProc = lpfnWndProc,
                       .cbClsExtra = cbClsExtra,
                       .cbWndExtra = cbWndExtra,
                       .hInstance = hInstance,
                       .hIcon = hIcon,
                       .hCursor = hCursor,
                       .hbrBackground = hbrBackground,
                       .lpszMenuName = lpszMenuName,
                       .lpszClassName = lpszClassName,
                       .hIconSm = hIconSm};
  ATOM const aClass{RegisterClassExW(&wc)};
  if (ClassRegistrationFailed(aClass)) {
    OutputDebugStringW(L"Warning: class was not registered\n");
#ifdef DCONSOLE
    std::wcerr << L"Class with name'" << lpszClassName 
               << "' was not registered\n";
    std::wcerr << L"Error code: " << GetLastError() << "\n";
#endif  // DCONSOLE
    //return GetLastError();
  }
}

gpu_renderer::WindowClass::~WindowClass() noexcept {
  if (!UnregisterClassW(lpClassName, hInstance)) {
    OutputDebugStringW(L"Warning: class was not unregistered\n");
#ifdef DCONSOLE
    //std::wcerr << L"Class with name'" << lpClassName
    //           << "' was not unregistered\n";
    //std::wcerr << L"Error code: " << GetLastError() << "\n";
#endif  // DCONSOLE
    //return GetLastError();
  }
}
