#include "GUI.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"

#include "Config.h"
#include "Hooks.h"
#include "ImGuiCustom.h"

#include <array>
#include <ctime>
#include <iomanip>
#include <Pdh.h>
#include <sstream>
#include <vector>
#include <Windows.h>

#pragma comment(lib, "pdh.lib")

static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;

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

    PdhOpenQueryW(nullptr, NULL, &cpuQuery);
    PdhAddEnglishCounterW(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
    PdhCollectQueryData(cpuQuery);
}

void GUI::render() noexcept
{
    const auto time = std::time(nullptr);
    const auto localTime = std::localtime(&time);

    static auto lastSecond = 0;
    static PDH_FMT_COUNTERVALUE cpuUsage;

    if (lastSecond != localTime->tm_sec) {
        lastSecond = localTime->tm_sec;

        PdhCollectQueryData(cpuQuery);
        PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_LONG, nullptr, &cpuUsage);
    }

    const auto windowTitle = std::ostringstream{ } << "GOESP [" << std::setw(2) << std::setfill('0') << localTime->tm_hour << ':' << std::setw(2) << std::setfill('0') << localTime->tm_min << ':' << std::setw(2) << std::setfill('0') << localTime->tm_sec << ']' << std::string(52, ' ') << "CPU: " << cpuUsage.longValue << "%###window";

    ImGui::SetNextWindowCollapsed(true, ImGui::GetIO().KeysDown[VK_SUBTRACT] ? ImGuiCond_Always : ImGuiCond_FirstUseEver);

    blockInput = ImGui::Begin(windowTitle.str().c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (ImGui::BeginTabBar("##tabbar", ImGuiTabBarFlags_Reorderable)) {
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 350.0f);

        ImGui::TextUnformatted("Build date: " __DATE__ " " __TIME__);
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 55.0f);

        if (ImGui::Button("Unload")) {
            PdhCloseQuery(cpuQuery);
            hooks.restore();
        }

        if (ImGui::BeginTabItem("ESP")) {
            static int currentCategory = 0;
            static int currentItem = 0;
            static int currentSubItem = 0;

            constexpr auto getConfig = [](int category, int item, int subItem = 0) constexpr noexcept -> Config::Shared& {
                switch (category) {
                case 0:
                    return config.players[item];
                case 1:
                    return config.players[item + 3];
                case 2:
                    switch (item) {
                    default:
                    case 0:
                        return config.weapons;
                    case 1:
                        return config.pistols[subItem];
                    case 2:
                        return config.smgs[subItem];
                    case 3:
                        return config.rifles[subItem];
                    case 4:
                        return config.sniperRifles[subItem];
                    case 5:
                        return config.shotguns[subItem];
                    case 6:
                        return config.heavy[subItem];
                    }
                case 3:
                    return config.misc[item];
                default:
                    return config.players[0];
                }
            };

            if (ImGui::ListBoxHeader("##list", { 155.0f, 300.0f })) {
                static constexpr std::array categories{ "Allies", "Enemies", "Weapons", "Misc" };

                for (size_t i = 0; i < categories.size(); i++) {
                    if (ImGui::Selectable(categories[i], currentCategory == i && currentItem == 0)) {
                        currentCategory = i;
                        currentItem = 0;
                    }

                    if (getConfig(i, 0).enabled)
                        continue;

                    constexpr auto getItems = [](int index) noexcept -> std::vector<const char*> {
                        switch (index) {
                        case 0:
                        case 1:
                            return { "Visible", "Occluded" };
                        case 2:
                            return { "Pistols", "SMGs", "Rifles", "Sniper Rifles", "Shotguns", "Heavy" };
                        default:
                            return { "Defuse Kits", "Chickens", "Planted C4" };
                        }
                    };

                    ImGui::PushID(i);
                    ImGui::Indent();

                    auto items = getItems(i);
                    for (size_t j = 0; j < items.size(); j++) {
                        if (ImGui::Selectable(items[j], currentCategory == i && currentItem == j + 1 && currentSubItem == 0)) {
                            currentCategory = i;
                            currentItem = j + 1;
                            currentSubItem = 0;
                        }

                        constexpr auto getSubItems = [](int category, int item) noexcept -> std::vector<const char*> {
                            switch (category) {
                            case 2:
                                switch (item) {
                                case 0:
                                    return { "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-SeveN", "CZ75-Auto", "Desert Eagle", "R8 Revolver" };
                                case 1:
                                    return { "MAC-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };
                                case 2:
                                    return { "Galil AR", "FAMAS", "AK-47", "M4A4", "M4A1-S", "SG 553", "AUG" };
                                case 3:
                                    return { "SSG 08", "AWP", "G3SG1", "SCAR-20" };
                                case 4:
                                    return { "Nova", "XM1014", "Sawed-Off", "MAG-7" };
                                case 5:
                                    return { "M249", "Negev" };
                                }
                            default:
                                return { };
                            }
                        };

                        if (getConfig(i, j + 1).enabled)
                            continue;

                        ImGui::Indent();

                        auto subItems = getSubItems(i, j);
                        for (size_t k = 0; k < subItems.size(); k++) {
                            if (ImGui::Selectable(subItems[k], currentCategory == i && currentItem == j + 1 && currentSubItem == k + 1)) {
                                currentCategory = i;
                                currentItem = j + 1;
                                currentSubItem = k + 1;
                            }
                        }

                        ImGui::Unindent();
                    }

                    ImGui::Unindent();
                    ImGui::PopID();
                }
                ImGui::ListBoxFooter();
            }

            ImGui::SameLine();

            if (ImGui::BeginChild("##child", { 400.0f, 0.0f })) {
                switch (currentCategory) {
                case 0:
                case 1: {
                    auto& playerConfig = config.players[currentCategory * 3 + currentItem];

                    ImGui::Checkbox("Enabled", &playerConfig.enabled);
                    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 260.0f);
                    ImGui::SetNextItemWidth(220.0f);
                    if (ImGui::BeginCombo("Font", config.systemFonts[playerConfig.fontIndex].first.c_str())) {
                        for (size_t i = 0; i < config.systemFonts.size(); i++) {
                            bool isSelected = config.systemFonts[i].second == playerConfig.font;
                            if (ImGui::Selectable(config.systemFonts[i].first.c_str(), isSelected, 0, { 300.0f, 0.0f })) {
                                playerConfig.fontIndex = i;
                                playerConfig.font = config.systemFonts[i].second;
                                config.scheduleFontLoad(playerConfig.font);
                            }
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Separator();

                    constexpr auto spacing{ 200.0f };
                    ImGuiCustom::colorPicker("Snaplines", playerConfig.snaplines);
                    ImGui::SameLine(spacing);
                    ImGuiCustom::colorPicker("Box", playerConfig.box);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(95.0f);
                    ImGui::Combo("", &playerConfig.boxType, "2D\0" "2D corners\0" "3D\0" "3D corners\0");
                    ImGuiCustom::colorPicker("Name", playerConfig.name);
                    break;
                }
                case 2: {
                    constexpr auto getWeaponConfig = [](int item, int subItem) constexpr noexcept -> Config::Weapon& {
                        switch (item) {
                        default:
                        case 0:
                            return config.weapons;
                        case 1:
                            return config.pistols[subItem];
                        case 2:
                            return config.smgs[subItem];
                        case 3:
                            return config.rifles[subItem];
                        case 4:
                            return config.sniperRifles[subItem];
                        case 5:
                            return config.shotguns[subItem];
                        case 6:
                            return config.heavy[subItem];
                        }
                    };

                    auto& weaponConfig = getWeaponConfig(currentItem, currentSubItem);

                    ImGui::Checkbox("Enabled", &weaponConfig.enabled);
                    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 260.0f);
                    ImGui::SetNextItemWidth(220.0f);
                    if (ImGui::BeginCombo("Font", config.systemFonts[weaponConfig.fontIndex].first.c_str())) {
                        for (size_t i = 0; i < config.systemFonts.size(); i++) {
                            bool isSelected = config.systemFonts[i].second == weaponConfig.font;
                            if (ImGui::Selectable(config.systemFonts[i].first.c_str(), isSelected, 0, { 300.0f, 0.0f })) {
                                weaponConfig.fontIndex = i;
                                weaponConfig.font = config.systemFonts[i].second;
                                config.scheduleFontLoad(weaponConfig.font);
                            }
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::Separator();

                    constexpr auto spacing{ 200.0f };
                    ImGuiCustom::colorPicker("Snaplines", weaponConfig.snaplines);
                    ImGui::SameLine(spacing);
                    ImGuiCustom::colorPicker("Box", weaponConfig.box);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(95.0f);
                    ImGui::Combo("", &weaponConfig.boxType, "2D\0" "2D corners\0" "3D\0" "3D corners\0");
                    ImGuiCustom::colorPicker("Name", weaponConfig.name);
                    ImGui::SameLine(spacing);
                    ImGuiCustom::colorPicker("Ammo", weaponConfig.ammo);
                    break;
                }
                case 3: {
                    auto& miscConfig = getConfig(currentCategory, currentItem, currentSubItem);

                    ImGui::Checkbox("Enabled", &miscConfig.enabled);
                    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 260.0f);
                    ImGui::SetNextItemWidth(220.0f);
                    if (ImGui::BeginCombo("Font", config.systemFonts[miscConfig.fontIndex].first.c_str())) {
                        for (size_t i = 0; i < config.systemFonts.size(); i++) {
                            bool isSelected = config.systemFonts[i].second == miscConfig.font;
                            if (ImGui::Selectable(config.systemFonts[i].first.c_str(), isSelected, 0, { 300.0f, 0.0f })) {
                                miscConfig.fontIndex = i;
                                miscConfig.font = config.systemFonts[i].second;
                                config.scheduleFontLoad(miscConfig.font);
                            }
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::Separator();

                    constexpr auto spacing{ 200.0f };
                    ImGuiCustom::colorPicker("Snaplines", miscConfig.snaplines);
                    ImGui::SameLine(spacing);
                    ImGuiCustom::colorPicker("Box", miscConfig.box);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(95.0f);
                    ImGui::Combo("", &miscConfig.boxType, "2D\0" "2D corners\0" "3D\0" "3D corners\0");
                    ImGuiCustom::colorPicker("Name", miscConfig.name);
                }
                }
                ImGui::EndChild();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Configs")) {
            if (ImGui::Button("Load"))
                config.load();
            if (ImGui::Button("Save"))
                config.save();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}
