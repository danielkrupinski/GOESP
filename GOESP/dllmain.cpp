#include "Hooks.h"

#ifdef _WIN32

#include <Windows.h>

extern "C" BOOL WINAPI _CRT_INIT(HMODULE module, DWORD reason, LPVOID reserved);

BOOL APIENTRY DllEntryPoint(HMODULE module, DWORD reason, LPVOID reserved)
{
    if (!_CRT_INIT(module, reason, reserved))
        return FALSE;

    if (reason == DLL_PROCESS_ATTACH) {
        hooks = std::make_unique<Hooks>(module);
        hooks->setup();
    }

    return TRUE;
}

#elif __linux__

void __attribute__((constructor)) DllEntryPoint()
{
    hooks = std::make_unique<Hooks>();
    hooks->setup();
}

#endif
