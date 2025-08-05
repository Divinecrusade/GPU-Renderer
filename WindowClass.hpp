#ifndef WINDOW_CLASS_HPP
#define WINDOW_CLASS_HPP

#include <string>
#include <unordered_map>

#include "OptimisedWindowsHeader.hpp"

namespace gpu_renderer {
class WindowClass {
 public:
  static constexpr HICON kDefaultIcon{NULL};
  static constexpr HCURSOR kDefaultCursor{NULL};
  static constexpr HBRUSH kNoBackground{NULL};
  static constexpr LPCWSTR kMenuName{NULL};

 public:
  WindowClass() = delete;
  WindowClass(UINT style, WNDPROC lpfnWndProc, int cbClsExtra, int cbWndExtra,
              HINSTANCE hInstance, HICON hIcon, HCURSOR hCursor,
              HBRUSH hbrBackground, LPCWSTR lpszMenuName, LPCWSTR lpszClassName,
              HICON hIconSm);
  WindowClass(WindowClass&) = delete;
  WindowClass(WindowClass&&) noexcept = delete;

  WindowClass& operator=(WindowClass const&) = delete;
  WindowClass& operator=(WindowClass&&) = delete;

  virtual ~WindowClass() noexcept;

  LPCWSTR GetLpClassName() const noexcept;

 private:
  struct wstring_hash {
    using is_transparent = void;

#pragma warning(push)
#pragma warning(disable : 4514)
    [[nodiscard]] size_t operator()(std::wstring_view txt) const noexcept {
      return std::hash<std::wstring_view>{}(txt);
    }
    [[nodiscard]] size_t operator()(const std::wstring& txt) const noexcept {
      return std::hash<std::wstring>{}(txt);
    }
#pragma warning(pop)
  };

 private:
  HINSTANCE hInstance_;
  std::wstring_view class_name_;

  static std::unordered_map<std::wstring, 
                            std::size_t, 
                            wstring_hash,
                            std::equal_to<>> class_ref_counts_;
};
}  // namespace gpu_renderer

#endif  // !WINDOW_CLASS_HPP
