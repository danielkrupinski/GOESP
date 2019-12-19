#include <Windows.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "Config.h"
#include "GUI.h"
#include "Interfaces.h"
#include "Memory.h"
#include "Hooks.h"

#include "SDK/InputSystem.h"

Config config{ "GOESP" };
GUI gui;
const Interfaces interfaces;
Memory memory;
Hooks hooks;

DWORD WINAPI waitOnUnload(HMODULE hModule)
{
    while (!hooks.readyForUnload())
        Sleep(50);

    interfaces.inputSystem->enableInput(true);
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        if (HANDLE thread = CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(waitOnUnload), hModule, 0, nullptr))
            CloseHandle(thread);
    }
    return TRUE;
}
