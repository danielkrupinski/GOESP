#pragma once

#ifdef _WIN32

#define __THISCALL __thiscall
#define __FASTCALL __fastcall
#define __CDECL __cdecl

#else

#define __THISCALL
#define __FASTCALL
#define __CDECL

#endif
