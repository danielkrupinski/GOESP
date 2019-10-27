#include "GUI.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"

#include "Hooks.h"

#include <ctime>
#include <iomanip>
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
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
}

void GUI::render() noexcept
{
    const auto time = std::time(nullptr);
    const auto localTime = std::localtime(&time);

    const auto windowTitle = std::ostringstream{ } << "GOESP [" << std::setw(2) << std::setfill('0') << localTime->tm_hour << ':' << std::setw(2) << std::setfill('0') << localTime->tm_min << ':' << std::setw(2) << std::setfill('0') << localTime->tm_sec << "]###window";

    blockInput = ImGui::Begin(windowTitle.str().c_str());

    ImGui::TextUnformatted("Build date: " __DATE__ " " __TIME__);
    ImGui::SameLine(0.0f, 30.0f);

    if (ImGui::Button("Unload"))
        hooks.restore();

    ImGui::Separator();
    ImGui::End();
}
