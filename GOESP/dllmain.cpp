#include <Windows.h>

#include "Config.h"
#include "EventListener.h"
#include "GUI.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "Memory.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        hooks = std::make_unique<Hooks>(hModule);

    return TRUE;
}
