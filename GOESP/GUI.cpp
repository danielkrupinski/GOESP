#include "GUI.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"

#include "Hooks.h"

#include <ctime>
#include <sstream>
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
    const auto time = std::time(nullptr);
    const auto localTime = std::localtime(&time);

    const auto windowTitle = std::ostringstream{ } << "GOESP [" << localTime->tm_hour << ':' << localTime->tm_min << ':' << localTime->tm_sec << "]###window";

    if (ImGui::Begin(windowTitle.str().c_str())) {
        blockInput = true;

        if (ImGui::Button("Unload"))
            hooks.restore();
        ImGui::End();
    } else {
        blockInput = false;
    }
}
