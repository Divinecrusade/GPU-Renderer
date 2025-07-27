#ifdef _DEBUG
#define _ALLOW_RTCc_IN_STL
#include <cassert>
#define __CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define DEBUG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif  // _DEBUG

#define _WIN32_WINNT 0x0A00  // Windows 10
#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define STRICT

#define NOGDICAPMASKS
#define NOMENUS
#define NOICONS
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NONLS
#define NOMETAFILE
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE
#pragma warning(push)
#pragma warning(disable : 5039 4668)
#include <Windows.h>
#pragma warning(pop)

[[maybe_unused]] int WINAPI wWinMain([[maybe_unused]] _In_ HINSTANCE,
                                     [[maybe_unused]] _In_opt_ HINSTANCE,
                                     [[maybe_unused]] _In_ LPWSTR,
                                     [[maybe_unused]] _In_ int) {
  static constexpr int EXIT_SUCCESS{0};

#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | 
                 _CRTDBG_LEAK_CHECK_DF |
                 _CRTDBG_CHECK_ALWAYS_DF);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif

  assert(_CrtDumpMemoryLeaks() == FALSE);
  return EXIT_SUCCESS;
}
