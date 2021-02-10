#pragma once

#ifdef _WIN32

#define __THISCALL __thiscall
#define __FASTCALL __fastcall
#define __CDECL __cdecl
#define __STDCALL __stdcall

#else

#define __THISCALL
#define __FASTCALL
#define __CDECL
#define __STDCALL

#endif

#ifdef _WIN32

constexpr auto CLIENT_DLL = "client";
constexpr auto ENGINE_DLL = "engine";
constexpr auto INPUTSYSTEM_DLL = "inputsystem";
constexpr auto LOCALIZE_DLL = "localize";
constexpr auto VSTDLIB_DLL = "vstdlib";
constexpr auto TIER0_DLL = "tier0";

#elif __linux__

constexpr auto CLIENT_DLL = "csgo/bin/linux64/client_client.so";
constexpr auto ENGINE_DLL = "engine_client.so";
constexpr auto INPUTSYSTEM_DLL = "inputsystem_client.so";
constexpr auto LOCALIZE_DLL = "localize_client.so";
constexpr auto VSTDLIB_DLL = "libvstdlib_client.so";
constexpr auto TIER0_DLL = "libtier0_client.so";

#elif __APPLE__

constexpr auto CLIENT_DLL = "csgo/bin/osx64/client.dylib";
constexpr auto ENGINE_DLL = "engine.dylib";
constexpr auto INPUTSYSTEM_DLL = "inputsystem.dylib";
constexpr auto LOCALIZE_DLL = "localize.dylib";
constexpr auto VSTDLIB_DLL = "libvstdlib.dylib";
constexpr auto TIER0_DLL = "libtier0.dylib";

#endif

#ifdef _WIN32
#define IS_WIN32() true
#define WIN32_UNIX(win32, unix) win32
#else
#define IS_WIN32() false
#define WIN32_UNIX(win32, unix) unix
#endif
