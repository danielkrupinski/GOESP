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

GUI::GUI() noexcept
{
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(FindWindowW(L"Valve001", nullptr));

    ImGui::StyleColorsClassic();
    ImGuiStyle& style = ImGui::GetStyle();

    style.ScrollbarSize = 13.0f;
    style.WindowTitleAlign = { 0.5f, 0.5f };

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
}

void GUI::render() noexcept
{
    if (!open)
        return;

    ImGui::Begin("GOESP", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

    if (!ImGui::BeginTabBar("##tabbar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_NoTooltip)) {
        ImGui::End();
        return;
    }

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 350.0f);

    ImGui::TextUnformatted("Build date: " __DATE__ " " __TIME__);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 55.0f);

    if (ImGui::Button("Unload"))
        hooks->restore();

    if (ImGui::BeginTabItem("ESP")) {
        static std::size_t currentCategory = 0;
        static std::size_t currentItem = 0;
        static std::size_t currentSubItem = 0;

        constexpr auto getConfig = [](int category, int item, int subItem = 0) constexpr noexcept -> Shared& {
            switch (category) {
            case 0:
                return config->players[item];
            case 1:
                return config->players[item + 3];
            case 2:
                switch (item) {
                default:
                case 0:
                    return config->weapons;
                case 1:
                    return config->pistols[subItem];
                case 2:
                    return config->smgs[subItem];
                case 3:
                    return config->rifles[subItem];
                case 4:
                    return config->sniperRifles[subItem];
                case 5:
                    return config->shotguns[subItem];
                case 6:
                    return config->machineguns[subItem];
                case 7:
                    return config->grenades[subItem];
                }
            case 3:
                return config->projectiles[item];
            case 4:
                return config->otherEntities[item];
            default:
                return config->players[0];
            }
        };

        if (ImGui::ListBoxHeader("##list", { 170.0f, 300.0f })) {
            constexpr std::array categories{ "Allies", "Enemies", "Weapons", "Projectiles", "Other Entities" };

            for (std::size_t i = 0; i < categories.size(); ++i) {
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
                        return { "Pistols", "SMGs", "Rifles", "Sniper Rifles", "Shotguns", "Machineguns", "Grenades" };
                    case 3:
                        return { "Flashbang", "HE Grenade", "Breach Charge", "Bump Mine", "Decoy Grenade", "Molotov", "TA Grenade", "Smoke Grenade", "Snowball" };
                    case 4:
                        return { "Defuse Kit", "Chicken", "Planted C4" };
                    default:
                        return { };
                    }
                };

                ImGui::PushID(i);
                ImGui::Indent();

                const auto items = getItems(i);
                for (std::size_t j = 0; j < items.size(); ++j) {
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
                            case 6:
                                return { "Flashbang", "HE Grenade", "Smoke Grenade", "Molotov", "Decoy Grenade", "Incendiary", "TA Grenade", "Fire Bomb", "Diversion", "Frag Grenade", "Snowball" };
                            }
                        default:
                            return { };
                        }
                    };

                    if (getConfig(i, j + 1).enabled)
                        continue;

                    ImGui::Indent();

                    const auto subItems = getSubItems(i, j);

                    for (std::size_t k = 0; k < subItems.size(); ++k) {
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
            auto& sharedConfig = getConfig(currentCategory, currentItem, currentSubItem);

            ImGui::Checkbox("Enabled", &sharedConfig.enabled);
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 260.0f);
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::BeginCombo("Font", config->systemFonts[sharedConfig.font.index].c_str())) {
                for (size_t i = 0; i < config->systemFonts.size(); i++) {
                    bool isSelected = config->systemFonts[i] == sharedConfig.font.name;
                    if (ImGui::Selectable(config->systemFonts[i].c_str(), isSelected, 0, { 250.0f, 0.0f })) {
                        sharedConfig.font.index = i;
                        sharedConfig.font.name = config->systemFonts[i];
                        sharedConfig.font.size = 15;
                        sharedConfig.font.fullName = sharedConfig.font.name + ' ' + std::to_string(sharedConfig.font.size);
                        config->scheduleFontLoad(sharedConfig.font.name);
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            constexpr auto spacing = 220.0f;
            ImGuiCustom::colorPicker("Snaplines", sharedConfig.snaplines);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.0f);
            ImGui::Combo("##1", &sharedConfig.snaplineType, "Bottom\0Top\0Crosshair\0");
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Box", sharedConfig.box);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(95.0f);
            ImGui::Combo("##2", &sharedConfig.boxType, "2D\0" "2D corners\0" "3D\0" "3D corners\0");
            ImGuiCustom::colorPicker("Name", sharedConfig.name);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Text Background", sharedConfig.textBackground);
            ImGui::SetNextItemWidth(95.0f);
            ImGui::InputFloat("Text Cull Distance", &sharedConfig.textCullDistance, 0.4f, 0.8f, "%.1fm");
            sharedConfig.textCullDistance = std::clamp(sharedConfig.textCullDistance, 0.0f, 999.9f);

            if (currentCategory < 2) {
                auto& playerConfig = config->players[currentCategory * 3 + currentItem];

                ImGuiCustom::colorPicker("Weapon", playerConfig.weapon);
                ImGui::SameLine(spacing);
                ImGuiCustom::colorPicker("Flash Duration", playerConfig.flashDuration);
            } else if (currentCategory == 2) {
                constexpr auto getWeaponConfig = [](int item, int subItem) constexpr noexcept -> Weapon& {
                    switch (item) {
                    default:
                    case 0:
                        return config->weapons;
                    case 1:
                        return config->pistols[subItem];
                    case 2:
                        return config->smgs[subItem];
                    case 3:
                        return config->rifles[subItem];
                    case 4:
                        return config->sniperRifles[subItem];
                    case 5:
                        return config->shotguns[subItem];
                    case 6:
                        return config->machineguns[subItem];
                    case 7:
                        return config->grenades[subItem];
                    }
                };
                auto& weaponConfig = getWeaponConfig(currentItem, currentSubItem);

                if (currentItem != 7)
                    ImGuiCustom::colorPicker("Ammo", weaponConfig.ammo);
            }

            ImGui::EndChild();
        }

        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Misc")) {
        ImGuiCustom::colorPicker("Reload Progress", config->reloadProgress);
        ImGuiCustom::colorPicker("Recoil Crosshair", config->recoilCrosshair);
        ImGui::Checkbox("Normalize Player Names", &config->normalizePlayerNames);
        ImGui::Checkbox("Purchase List", &config->purchaseList);
        ImGui::SameLine();

        if (ImGui::Button("..."))
            ImGui::OpenPopup("##purchaselist");

        if (ImGui::BeginPopup("##purchaselist")) {
            ImGui::SetNextItemWidth(75.0f);
            ImGui::Combo("Mode", &config->purchaseListMode, "Details\0Total\0");
            ImGui::Checkbox("Show Prices", &config->purchaseListPrices);
            ImGui::EndPopup();
        }
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Configs")) {
        ImGui::TextUnformatted("Config is saved as \"config.txt\" inside GOESP directory in Documents");
        if (ImGui::Button("Load"))
            config->load();
        if (ImGui::Button("Save"))
            config->save();
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
    ImGui::End();
}
