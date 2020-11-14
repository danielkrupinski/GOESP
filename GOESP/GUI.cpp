#include "GUI.h"

#include "imgui/imgui.h"

#include "Config.h"
#include "Hooks.h"
#include "ImGuiCustom.h"
#include "Hacks/ESP.h"
#include "Hacks/Misc.h"

#include <array>
#include <vector>

GUI::GUI() noexcept
{
    ImGui::StyleColorsClassic();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScrollbarSize = 13.0f;
    style.WindowTitleAlign = { 0.5f, 0.5f };
    style.Colors[ImGuiCol_WindowBg].w = 0.8f;

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
    io.Fonts->AddFontDefault();
}

void GUI::render() noexcept
{
    if (!open)
        return;

    ImGui::Begin(
        "GOESP for "
#ifdef _WIN32
        "Windows"
#elif __linux__
        "Linux"
#elif __APPLE__
        "macOS"
#else
#error("Unsupported platform!")
#endif
        , nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

    if (!ImGui::BeginTabBar("##tabbar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_NoTooltip)) {
        ImGui::End();
        return;
    }

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 350.0f);

    ImGui::TextUnformatted("Build date: " __DATE__ " " __TIME__);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 55.0f);

    if (ImGui::Button("Unload"))
        hooks->uninstall();

    if (ImGui::BeginTabItem("ESP")) {
        ESP::drawGUI();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Misc")) {
        Misc::drawGUI();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Configs")) {
#ifdef _WIN32
        ImGui::TextUnformatted("Config is saved as \"config.txt\" inside GOESP directory in Documents");
#elif __linux__
        ImGui::TextUnformatted("Config is saved as \"config.txt\" inside ~/GOESP directory");
#endif
        if (ImGui::Button("Load"))
            config->load();
        if (ImGui::Button("Save"))
            config->save();
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
    ImGui::End();
}

void GUI::drawMiscTab() noexcept
{

}
