#ifndef DEBUG_HEADER_HPP
#define DEBUG_HEADER_HPP

#ifdef _DEBUG
#define _ALLOW_RTCc_IN_STL
#define __CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define DEBUG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif  // _DEBUG

#endif  // !DEBUG_HEADER_HPP
