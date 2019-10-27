#include <intrin.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "Hooks.h"
#include "Memory.h"

static HRESULT __stdcall present(IDirect3DDevice9* device, const RECT* src, const RECT* dest, HWND windowOverride, const RGNDATA* dirtyRegion) noexcept
{
    hooks.present.hookCalled = true;

    static bool imguiInit{ ImGui_ImplDX9_Init(device) };

    IDirect3DVertexDeclaration9* vertexDeclaration;
    device->GetVertexDeclaration(&vertexDeclaration);

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::EndFrame();

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    device->SetVertexDeclaration(vertexDeclaration);
    vertexDeclaration->Release();

    auto result{ hooks.present.original(device, src, dest, windowOverride, dirtyRegion) };

    hooks.present.hookCalled = false;
    return result;
}

static HRESULT __stdcall reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
    hooks.reset.hookCalled = true;
    
    auto result{ hooks.reset.original(device, params) };

    hooks.reset.hookCalled = false;
    return result;
}

Hooks::Hooks() noexcept
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(FindWindowW(L"Valve001", NULL));

    present.original = **reinterpret_cast<decltype(present.original)**>(memory.present);
    **reinterpret_cast<decltype(::present)***>(memory.present) = ::present;

    reset.original = **reinterpret_cast<decltype(reset.original)**>(memory.reset);
    **reinterpret_cast<decltype(::reset)***>(memory.reset) = ::reset;
}

bool Hooks::readyForUnload() noexcept
{
    return unload && !(hooks.present.hookCalled || hooks.reset.hookCalled);
}

void Hooks::restore() noexcept
{
    **reinterpret_cast<void***>(memory.present) = present.original;
    **reinterpret_cast<void***>(memory.reset) = reset.original;

    unload = true;
}
