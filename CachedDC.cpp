#include "CachedDC.hpp"

gpu_renderer::CachedDC::CachedDC(HINSTANCE hInstance, WNDPROC lpfnWndProc) noexcept
    : WindowClass{CS_OWNDC,
                  lpfnWndProc,
                  0,
                  0,
                  hInstance,
                  WindowClass::kDefaultIcon,
                  WindowClass::kDefaultCursor,
                  WindowClass::kNoBackground,
                  WindowClass::kMenuName,
                  kClassName,
                  WindowClass::kDefaultIcon} {}
