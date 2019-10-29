#include "GUI.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"

#include "Hooks.h"

#include <array>
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

    static int currentCategory = 0;
    static int currentItem = 0;

    if (ImGui::ListBoxHeader("##list", { 125.0f, 300.0f })) {
        static constexpr std::array categories{ "Allies", "Enemies" };

        for (size_t i = 0; i < categories.size(); i++) {
            if (ImGui::Selectable(categories[i], currentCategory == i && currentItem == 0)) {
                currentCategory = i;
                currentItem = 0;
            }
            ImGui::PushID(i);

            constexpr auto getItems = [](int index) constexpr noexcept -> std::initializer_list<const char*> {
                switch (index) {
                case 0:
                case 1:
                    return { "Visible", "Occluded" };
                case 2:
                    // return { "AK-47", "Tec-9" };
                case 3:
                    // return { "Smoke", "HE Grenade", "Flashbang" };
                default:
                    return { };
                }
            };

            ImGui::Indent();

            auto items = getItems(i);
            for (auto it = items.begin(); it != items.end(); it++) {
                auto itemIndex = std::distance(items.begin(), it) + 1;

                if (ImGui::Selectable(*it, currentCategory == i && currentItem == itemIndex)) {
                    currentCategory = i;
                    currentItem = itemIndex;
                }
            }

            ImGui::Unindent();
            ImGui::PopID();
        }
        ImGui::ListBoxFooter();
    }

    ImGui::Separator();

    ImGui::TextUnformatted("Build date: " __DATE__ " " __TIME__);
    ImGui::SameLine(0.0f, 30.0f);

    if (ImGui::Button("Unload"))
        hooks.restore();

    ImGui::End();
}
