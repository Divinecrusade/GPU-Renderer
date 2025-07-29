#include "CachedDC.hpp"

gpu_renderer::CachedDC::CachedDC(HINSTANCE hInstance, WNDPROC lpfnWndProc) noexcept
    : WindowClass{CS_OWNDC,
                  lpfnWndProc,
                  0,
                  0,
                  hInstance,
                  WindowClass::DEFAULT_ICON,
                  WindowClass::DEFAULT_CURSOR,
                  WindowClass::NO_BACKGROUND,
                  WindowClass::NO_MENU_NAME,
                  CLASS_NAME,
                  WindowClass::DEFAULT_ICON} {}
