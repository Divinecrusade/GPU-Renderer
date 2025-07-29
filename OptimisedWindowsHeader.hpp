#ifndef WINDOWS_HPP
#define WINDOWS_HPP

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
#pragma warning(disable : 4005 4668 5039 4514 4820 4711)
#pragma warning(disable : 6001 6011 6387)
#include <Windows.h>
#pragma warning(pop)

#endif  // !WINDOWS_HPP
