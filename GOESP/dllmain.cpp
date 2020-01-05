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

static WNDPROC originalWndproc;
static HMODULE module;

static LRESULT WINAPI init(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    SetWindowLongPtr(FindWindowW(L"Valve001", nullptr), GWLP_WNDPROC, LONG_PTR(originalWndproc));
    config = std::make_unique<Config>("GOESP");
    gui = std::make_unique<GUI>();
    interfaces = std::make_unique<const Interfaces>();
    memory = std::make_unique<Memory>();
    hooks = std::make_unique<Hooks>(module);

    return CallWindowProc(originalWndproc, window, msg, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        module = hModule;
        originalWndproc = WNDPROC(SetWindowLongPtrA(FindWindowW(L"Valve001", nullptr), GWLP_WNDPROC, LONG_PTR(init)));
    }
    return TRUE;
}
