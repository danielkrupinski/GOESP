#include "GUI.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"

#include "Hooks.h"

#include <Windows.h>

GUI::GUI() noexcept
{
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(FindWindowW(L"Valve001", nullptr));

    ImGui::StyleColorsClassic();
    ImGuiStyle& style = ImGui::GetStyle();

    style.ScrollbarSize = 9.0f;

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
}

void GUI::render() noexcept
{
    if (ImGui::Begin("GOESP")) {
        if (ImGui::Button("Unload"))
            hooks.restore();
        ImGui::End();
    }
}
