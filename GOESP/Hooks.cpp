#include "Hooks.h"

#include <intrin.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "Config.h"
#include "GUI.h"
#include "Hacks/ESP.h"
#include "Hacks/Misc.h"
#include "Interfaces.h"
#include "Memory.h"

#include "SDK/InputSystem.h"

static LRESULT __stdcall wndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    hooks->wndProc.hookCalled = true;

    LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam);
    interfaces->inputSystem->enableInput(!gui.blockInput);

    auto result = CallWindowProc(hooks->wndProc.original, window, msg, wParam, lParam);

    hooks->wndProc.hookCalled = false;
    return result;
}

static HRESULT __stdcall present(IDirect3DDevice9* device, const RECT* src, const RECT* dest, HWND windowOverride, const RGNDATA* dirtyRegion) noexcept
{
    hooks->present.hookCalled = true;

    static auto _ = ImGui_ImplDX9_Init(device);

    IDirect3DVertexDeclaration9* vertexDeclaration;
    device->GetVertexDeclaration(&vertexDeclaration);

    if (config.loadScheduledFonts()) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        ImGui_ImplDX9_CreateDeviceObjects();
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ESP::render(ImGui::GetBackgroundDrawList());
    Misc::drawReloadProgress(ImGui::GetBackgroundDrawList());
    Misc::drawRecoilCrosshair(ImGui::GetBackgroundDrawList());

    gui.render();

    ImGui::EndFrame();

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    device->SetVertexDeclaration(vertexDeclaration);
    vertexDeclaration->Release();

    auto result = hooks->present.original(device, src, dest, windowOverride, dirtyRegion);

    hooks->present.hookCalled = false;
    return result;
}

static HRESULT __stdcall reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
    hooks->reset.hookCalled = true;

    ImGui_ImplDX9_InvalidateDeviceObjects();
    auto result = hooks->reset.original(device, params);
    ImGui_ImplDX9_CreateDeviceObjects();

    hooks->reset.hookCalled = false;
    return result;
}

Hooks::Hooks() noexcept
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    wndProc.original = WNDPROC(SetWindowLongPtrA(FindWindowW(L"Valve001", nullptr), GWLP_WNDPROC, LONG_PTR(::wndProc)));

    present.original = **reinterpret_cast<decltype(present.original)**>(memory->present);
    **reinterpret_cast<decltype(::present)***>(memory->present) = ::present;

    reset.original = **reinterpret_cast<decltype(reset.original)**>(memory->reset);
    **reinterpret_cast<decltype(::reset)***>(memory->reset) = ::reset;
}

void Hooks::restore() noexcept
{
    **reinterpret_cast<void***>(memory->present) = present.original;
    **reinterpret_cast<void***>(memory->reset) = reset.original;

    SetWindowLongPtrA(FindWindowW(L"Valve001", nullptr), GWLP_WNDPROC, LONG_PTR(wndProc.original));

    unload = true;
}
