#include "WindowClass.hpp"

#include "DebugHeader.hpp"

#ifdef LOG_WINDOW_CLASS
#include <iostream>
#endif  // LOG_WINDOW_CLASS
#include <cassert>

std::unordered_map<std::wstring, std::size_t,
                   gpu_renderer::WindowClass::wstring_hash, std::equal_to<>>
    gpu_renderer::WindowClass::class_ref_counts_{};

gpu_renderer::WindowClass::WindowClass(
    UINT style, WNDPROC lpfnWndProc, int cbClsExtra, int cbWndExtra,
    HINSTANCE hInstance, HICON hIcon, HCURSOR hCursor, HBRUSH hbrBackground,
    LPCWSTR lpszMenuName, LPCWSTR lpszClassName, HICON hIconSm) noexcept
    : hInstance_{hInstance} {
  assert(((void)"Class name cannot be nullptr", lpszClassName != nullptr));
  assert(((void)"Class name should not be empty",
          std::wcslen(lpszClassName) > 0ull));
  assert(((void)"Window procedure cannot be null", lpfnWndProc != nullptr));
  assert(((void)"Instance handle cannot be null", hInstance != NULL));

  auto const [it, _] = class_ref_counts_.try_emplace(lpszClassName, 0ull);
  class_name_ = it->first;

  assert(((void)"string_view must reference map key",
          class_name_.data() == it->first.data()));
  assert(((void)"string_view size must match map key size",
          class_name_.size() == it->first.size()));

  if (it->second == 0ull) {
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
                         .lpszClassName = class_name_.data(),
                         .hIconSm = hIconSm};

    if (ATOM const aClass{RegisterClassExW(&wc)};
        ClassRegistrationFailed(aClass)) {
#ifdef LOG_WINDOW_CLASS
      std::wcerr << L"Class with name '" << class_name_
                 << L"' was not registered\n";
      std::wcerr << L"Error code: " << GetLastError() << L"\n";
#endif  // LOG_WINDOW_CLASS
      assert(((void)"Reference count must remain zero on registration failure",
              it->second == 0ull));
      return;
    } else {
#ifdef LOG_WINDOW_CLASS
      std::wclog << L"Class '" << class_name_ << L"' registered successfully\n";
#endif  // LOG_WINDOW_CLASS
    }
  }

  ++it->second;

#ifdef LOG_WINDOW_CLASS
  std::wclog << L"Class '" << class_name_ << L"' ref count: " << it->second
             << L"\n";
#endif  // LOG_WINDOW_CLASS
}

gpu_renderer::WindowClass::~WindowClass() noexcept {
  assert(
      ((void)"Class name must be valid in destructor", !class_name_.empty()));

  if (auto const it = class_ref_counts_.find(class_name_);
      it != class_ref_counts_.end()) {
    assert(((void)"string_view must still reference map key",
            class_name_.data() == it->first.data()));
    assert(((void)"Reference count must be positive before decrement",
            it->second > 0ull));
    --it->second;
#ifdef LOG_WINDOW_CLASS
    std::wclog << L"Class '" << class_name_ << L"' ref count: " << it->second
               << L"\n";
#endif  // LOG_WINDOW_CLASS

    if (it->second == 0) {
      if (!UnregisterClassW(class_name_.data(), hInstance_)) {
#ifdef LOG_WINDOW_CLASS
        std::wcerr << L"Class with name '" << class_name_
                   << L"' was not unregistered\n";
        std::wcerr << L"Error code: " << GetLastError() << L"\n";
#endif  // LOG_WINDOW_CLASS
      } 
#ifdef LOG_WINDOW_CLASS
      else {
        std::wclog << L"Class '" << class_name_
                   << L"' unregistered successfully\n";
      }
#endif  // LOG_WINDOW_CLASS
      assert(((void)"Reference count must be zero before map erase",
              it->second == 0ull));
      class_ref_counts_.erase(it);
    }
  } else {
    assert(((void)"Class name not found in reference count map during "
                  "destruction",
            false));
  }
}

LPCWSTR gpu_renderer::WindowClass::GetLpClassName() const noexcept {
  return class_name_.data();
}
