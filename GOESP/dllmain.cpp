#include <Windows.h>

#include "GUI.h"
#include "Interfaces.h"
#include "Memory.h"
#include "Hooks.h"

#include "SDK/InputSystem.h"

GUI gui;
Interfaces interfaces;
Memory memory;
Hooks hooks;

DWORD WINAPI waitOnUnload(HMODULE hModule)
{
    while (!hooks.readyForUnload())
        Sleep(50);

    interfaces.inputSystem->enableInput(true);

    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CloseHandle(CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(waitOnUnload), hModule, 0, nullptr));
    }
    return TRUE;
}
