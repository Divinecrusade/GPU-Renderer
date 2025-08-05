#include "CachedDC.hpp"
#include "resource.h"

gpu_renderer::CachedDC::CachedDC(HINSTANCE hInstance, WNDPROC lpfnWndProc)
    : WindowClass{CS_OWNDC,
                  lpfnWndProc,
                  0,
                  0,
                  hInstance,
                  LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)),
                  WindowClass::kDefaultCursor,
                  WindowClass::kNoBackground,
                  WindowClass::kMenuName,
                  kClassName,
                  LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP_ICON))} 
{}
