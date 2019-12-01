#include "ESP.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "Config.h"
#include "Interfaces.h"
#include "Memory.h"
#include "SDK/ClassId.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/GlobalVars.h"
#include "SDK/Localize.h"
#include "SDK/Vector.h"
#include "SDK/WeaponData.h"
#include "SDK/WeaponId.h"

static constexpr auto rainbowColor(float time, float speed, float alpha) noexcept
{
    return std::make_tuple(std::sin(speed * time) * 0.5f + 0.5f,
                           std::sin(speed * time + static_cast<float>(2 * M_PI / 3)) * 0.5f + 0.5f,
                           std::sin(speed * time + static_cast<float>(4 * M_PI / 3)) * 0.5f + 0.5f,
                           alpha);
}

static constexpr bool worldToScreen(const Vector& in, ImVec2& out) noexcept
{
    const auto matrix = memory.viewMatrix;

    float w = matrix->_41 * in.x + matrix->_42 * in.y + matrix->_43 * in.z + matrix->_44;

    if (w > 0.001f) {
        const auto [width, height] = interfaces.engine->getScreenSize();
        out.x = width / 2 * (1 + (matrix->_11 * in.x + matrix->_12 * in.y + matrix->_13 * in.z + matrix->_14) / w);
        out.y = height / 2 * (1 - (matrix->_21 * in.x + matrix->_22 * in.y + matrix->_23 * in.z + matrix->_24) / w);
        return true;
    }
    return false;
}

struct BoundingBox {
    ImVec2 min, max;
    ImVec2 vertices[8];
};

static auto boundingBox(Entity* entity, BoundingBox& out) noexcept
{
    const auto [width, height] = interfaces.engine->getScreenSize();

    out.min.x = static_cast<float>(width * 2);
    out.min.y = static_cast<float>(height * 2);
    out.max.x = -static_cast<float>(width * 2);
    out.max.y = -static_cast<float>(height * 2);

    const auto mins = entity->getCollideable()->obbMins();
    const auto maxs = entity->getCollideable()->obbMaxs();

    for (int i = 0; i < 8; ++i) {
        const Vector point{ i & 1 ? maxs.x : mins.x,
                            i & 2 ? maxs.y : mins.y,
                            i & 4 ? maxs.z : mins.z };

        if (!worldToScreen(point.transform(entity->coordinateFrame()), out.vertices[i]))
            return false;

        if (out.min.x > out.vertices[i].x)
            out.min.x = out.vertices[i].x;

        if (out.min.y > out.vertices[i].y)
            out.min.y = out.vertices[i].y;

        if (out.max.x < out.vertices[i].x)
            out.max.x = out.vertices[i].x;

        if (out.max.y < out.vertices[i].y)
            out.max.y = out.vertices[i].y;
    }
    return true;
}

static void renderBox(ImDrawList* drawList, Entity* entity, const BoundingBox& bbox, const Config::Shared& config) noexcept
{
    if (config.box.enabled) {
        ImU32 color = config.box.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.box.rainbowSpeed, config.box.color[3])) : ImGui::ColorConvertFloat4ToU32(config.box.color);

        switch (config.boxType) {
        case 0:
            drawList->AddRect(bbox.min, bbox.max, color, config.box.rounding);
            break;

        case 1:
            drawList->AddLine(bbox.min, { bbox.min.x, bbox.min.y + (bbox.max.y - bbox.min.y) * 0.25f }, color);
            drawList->AddLine(bbox.min, { bbox.min.x + (bbox.max.x - bbox.min.x) * 0.25f, bbox.min.y }, color);

            drawList->AddLine({ bbox.max.x, bbox.min.y }, { bbox.max.x - (bbox.max.x - bbox.min.x) * 0.25f, bbox.min.y }, color);
            drawList->AddLine({ bbox.max.x, bbox.min.y }, { bbox.max.x, bbox.min.y + (bbox.max.y - bbox.min.y) * 0.25f }, color);

            drawList->AddLine({ bbox.min.x, bbox.max.y }, { bbox.min.x, bbox.max.y - (bbox.max.y - bbox.min.y) * 0.25f }, color);
            drawList->AddLine({ bbox.min.x, bbox.max.y }, { bbox.min.x + (bbox.max.x - bbox.min.x) * 0.25f, bbox.max.y }, color);

            drawList->AddLine(bbox.max, { bbox.max.x - (bbox.max.x - bbox.min.x) * 0.25f, bbox.max.y }, color);
            drawList->AddLine(bbox.max, { bbox.max.x, bbox.max.y - (bbox.max.y - bbox.min.y) * 0.25f }, color);
            break;
        case 2:
            for (int i = 0; i < 8; ++i) {
                for (int j = 1; j <= 4; j <<= 1) {
                    if (!(i & j))
                        drawList->AddLine(bbox.vertices[i], bbox.vertices[i + j], color);
                }
            }
            break;
        case 3:
            for (int i = 0; i < 8; ++i) {
                for (int j = 1; j <= 4; j <<= 1) {
                    if (!(i & j)) {
                        drawList->AddLine(bbox.vertices[i], { bbox.vertices[i].x + (bbox.vertices[i + j].x - bbox.vertices[i].x) * 0.25f, bbox.vertices[i].y + (bbox.vertices[i + j].y - bbox.vertices[i].y) * 0.25f }, color);
                        drawList->AddLine({ bbox.vertices[i].x + (bbox.vertices[i + j].x - bbox.vertices[i].x) * 0.75f, bbox.vertices[i].y + (bbox.vertices[i + j].y - bbox.vertices[i].y) * 0.75f }, bbox.vertices[i + j], color);
                    }
                }
            }
            break;
        }
    }
}

static void renderPlayerBox(ImDrawList* drawList, Entity* entity, const Config::Player& config) noexcept
{
    if (BoundingBox bbox; boundingBox(entity, bbox)) {
        renderBox(drawList, entity, bbox, config);

        if (config.name.enabled) {
            if (PlayerInfo playerInfo; interfaces.engine->getPlayerInfo(entity->index(), playerInfo)) {
                ImGui::PushFont(::config.fonts[config.font]);

                const auto distance = (interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer())->getAbsOrigin() - entity->getAbsOrigin()).length();
                const auto fontSize = std::clamp(15.0f * 10.0f / std::sqrt(distance), 10.0f, 15.0f);
                const auto oldFontSize = ImGui::GetCurrentContext()->FontSize;

                ImGui::GetCurrentContext()->FontSize = fontSize;
                const auto textSize = ImGui::CalcTextSize(playerInfo.name);
                if (config.textBackground.enabled) {
                    const ImU32 color = config.textBackground.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.textBackground.rainbowSpeed, config.textBackground.color[3])) : ImGui::ColorConvertFloat4ToU32(config.textBackground.color);
                    drawList->AddRectFilled({ bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2 - 2, bbox.min.y - 7 - textSize.y }, { bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2 + textSize.x + 2, bbox.min.y - 3 }, color, config.textBackground.rounding);
                }
                const ImU32 color = config.name.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.name.rainbowSpeed, config.name.color[3])) : ImGui::ColorConvertFloat4ToU32(config.name.color);
                drawList->AddText(nullptr, fontSize, { bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2, bbox.min.y - 5 - textSize.y }, color, playerInfo.name);
                ImGui::GetCurrentContext()->FontSize = oldFontSize;

                ImGui::PopFont();
            }
        }
    }
}

static void renderWeaponBox(ImDrawList* drawList, Entity* entity, const Config::Weapon& config) noexcept
{
    if (BoundingBox bbox; boundingBox(entity, bbox)) {
        renderBox(drawList, entity, bbox, config);

        ImGui::PushFont(::config.fonts[config.font]);
        const auto oldFontSize = ImGui::GetCurrentContext()->FontSize;

        if (config.name.enabled) {
            if (const auto weaponData = entity->getWeaponData()) {
                if (char weaponName[100]; WideCharToMultiByte(CP_UTF8, 0, interfaces.localize->find(weaponData->name), -1, weaponName, _countof(weaponName), nullptr, nullptr)) {
                    const auto distance = (interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer())->getAbsOrigin() - entity->getAbsOrigin()).length();
                    const auto fontSize = std::clamp(15.0f * 10.0f / std::sqrt(distance), 10.0f, 15.0f);

                    ImGui::GetCurrentContext()->FontSize = fontSize;
                    const auto textSize = ImGui::CalcTextSize(weaponName);
                    if (config.textBackground.enabled) {
                        const ImU32 color = config.textBackground.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.textBackground.rainbowSpeed, config.textBackground.color[3])) : ImGui::ColorConvertFloat4ToU32(config.textBackground.color);
                        drawList->AddRectFilled({ bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2 - 2, bbox.min.y - 7 - textSize.y }, { bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2 + textSize.x + 2, bbox.min.y - 3 }, color, config.textBackground.rounding);
                    }
                    const ImU32 color = config.name.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.name.rainbowSpeed, config.name.color[3])) : ImGui::ColorConvertFloat4ToU32(config.name.color);
                    drawList->AddText(nullptr, fontSize, { bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2, bbox.min.y - 5 - textSize.y }, color, weaponName);
                }
            }
        }

        if (config.ammo.enabled) {
            const auto text{ std::to_string(entity->clip()) + " / " + std::to_string(entity->reserveAmmoCount()) };
            const auto distance = (interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer())->getAbsOrigin() - entity->getAbsOrigin()).length();
            const auto fontSize = std::clamp(15.0f * 10.0f / std::sqrt(distance), 10.0f, 15.0f);

            ImGui::GetCurrentContext()->FontSize = fontSize;
            const auto textSize = ImGui::CalcTextSize(text.c_str());
            if (config.textBackground.enabled) {
                const ImU32 color = config.textBackground.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.textBackground.rainbowSpeed, config.textBackground.color[3])) : ImGui::ColorConvertFloat4ToU32(config.textBackground.color);
                drawList->AddRectFilled({ bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2 - 2, bbox.max.y + 3 }, { bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2 + textSize.x + 2, bbox.max.y + 7 + textSize.y }, color, config.textBackground.rounding);
            }
            const ImU32 color = config.ammo.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.ammo.rainbowSpeed, config.ammo.color[3])) : ImGui::ColorConvertFloat4ToU32(config.ammo.color);
            drawList->AddText(nullptr, fontSize, { bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2, bbox.max.y + 5 }, color, text.c_str());
        }

        ImGui::GetCurrentContext()->FontSize = oldFontSize;
        ImGui::PopFont();
    }
}

static void renderEntityBox(ImDrawList* drawList, Entity* entity, const char* name, const Config::Shared& config) noexcept
{
    if (BoundingBox bbox; boundingBox(entity, bbox)) {
        renderBox(drawList, entity, bbox, config);

        ImGui::PushFont(::config.fonts[config.font]);
        const auto oldFontSize = ImGui::GetCurrentContext()->FontSize;

        if (config.name.enabled) {
            const auto distance = (interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer())->getAbsOrigin() - entity->getAbsOrigin()).length();
            const auto fontSize = std::clamp(15.0f * 10.0f / std::sqrt(distance), 10.0f, 15.0f);

            ImGui::GetCurrentContext()->FontSize = fontSize;
            const auto textSize = ImGui::CalcTextSize(name);
            if (config.textBackground.enabled) {
                const ImU32 color = config.textBackground.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.textBackground.rainbowSpeed, config.textBackground.color[3])) : ImGui::ColorConvertFloat4ToU32(config.textBackground.color);
                drawList->AddRectFilled({ bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2 - 2, bbox.min.y - 7 - textSize.y }, { bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2 + textSize.x + 2, bbox.min.y - 3 }, color, config.textBackground.rounding);
            }
            const ImU32 color = config.name.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.name.rainbowSpeed, config.name.color[3])) : ImGui::ColorConvertFloat4ToU32(config.name.color);
            drawList->AddText(nullptr, fontSize, { bbox.min.x + (bbox.max.x - bbox.min.x - textSize.x) / 2, bbox.min.y - 5 - textSize.y }, color, name);
        }

        ImGui::GetCurrentContext()->FontSize = oldFontSize;
        ImGui::PopFont();
    }
}

static void renderSnaplines(ImDrawList* drawList, Entity* entity, const Config::ColorToggle& config) noexcept
{
    if (config.enabled) {
        if (ImVec2 position; worldToScreen(entity->getAbsOrigin(), position)) {
            const auto [width, height] = interfaces.engine->getScreenSize();
            drawList->AddLine({ static_cast<float>(width / 2), static_cast<float>(height) }, position, config.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.rainbowSpeed, config.color[3])) : ImGui::ColorConvertFloat4ToU32(config.color));
        }
    }
}

enum EspId {
    ALLIES_ALL = 0,
    ALLIES_VISIBLE,
    ALLIES_OCCLUDED,

    ENEMIES_ALL,
    ENEMIES_VISIBLE,
    ENEMIES_OCCLUDED
};

static constexpr bool renderPlayerEsp(ImDrawList* drawList, Entity* entity, EspId id) noexcept
{
    if (config.players[id].enabled) {
        renderPlayerBox(drawList, entity, config.players[id]);
        renderSnaplines(drawList, entity, config.players[id].snaplines);
    }
    return config.players[id].enabled;
}

static constexpr void renderWeaponEsp(ImDrawList* drawList, Entity* entity, const Config::Weapon& parentConfig, const Config::Weapon& itemConfig) noexcept
{
    const auto& config = parentConfig.enabled ? parentConfig : itemConfig;
    if (config.enabled) {
        renderWeaponBox(drawList, entity, config);
        renderSnaplines(drawList, entity, config.snaplines);
    }
}

static void renderEntityEsp(ImDrawList* drawList, Entity* entity, const Config::Shared& parentConfig, const Config::Shared& itemConfig, const char* name, const wchar_t* wideName = nullptr) noexcept
{
    const auto& config = parentConfig.enabled ? parentConfig : itemConfig;

    if (config.enabled) {
        if (name)
            renderEntityBox(drawList, entity, name, config);
        else if (char nameConverted[100]; WideCharToMultiByte(CP_UTF8, 0, wideName, -1, nameConverted, _countof(nameConverted), nullptr, nullptr))
            renderEntityBox(drawList, entity, nameConverted, config);

        renderSnaplines(drawList, entity, config.snaplines);
    }
}

void ESP::render(ImDrawList* drawList) noexcept
{
    if (interfaces.engine->isInGame()) {
        const auto localPlayer = interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer());

        if (!localPlayer)
            return;

        for (int i = 1; i <= interfaces.engine->getMaxClients(); ++i) {
            const auto entity = interfaces.entityList->getEntity(i);
            if (!entity || entity == localPlayer || entity->isDormant() || !entity->isAlive())
                continue;

            if (!entity->isEnemy()) {
                if (!renderPlayerEsp(drawList, entity, ALLIES_ALL)) {
                    if (entity->isVisible())
                        renderPlayerEsp(drawList, entity, ALLIES_VISIBLE);
                    else
                        renderPlayerEsp(drawList, entity, ALLIES_OCCLUDED);
                }
            } else if (!renderPlayerEsp(drawList, entity, ENEMIES_ALL)) {
                if (entity->isVisible())
                    renderPlayerEsp(drawList, entity, ENEMIES_VISIBLE);
                else
                    renderPlayerEsp(drawList, entity, ENEMIES_OCCLUDED);
            }
        }

        for (int i = interfaces.engine->getMaxClients() + 1; i <= interfaces.entityList->getHighestEntityIndex(); ++i) {
            const auto entity = interfaces.entityList->getEntity(i);
            if (!entity || entity->isDormant())
                continue;

            if (entity->isWeapon() && entity->ownerEntity() == -1) {
                constexpr auto getWeaponIndex = [](WeaponId weaponId) constexpr noexcept {
                    switch (weaponId) {
                    default: return 0;

                    case WeaponId::Glock:
                    case WeaponId::Mac10:
                    case WeaponId::GalilAr:
                    case WeaponId::Ssg08:
                    case WeaponId::Nova:
                    case WeaponId::M249:
                        return 1;
                    case WeaponId::Hkp2000:
                    case WeaponId::Mp9:
                    case WeaponId::Famas:
                    case WeaponId::Awp:
                    case WeaponId::Xm1014:
                    case WeaponId::Negev:
                        return 2;
                    case WeaponId::Usp_s:
                    case WeaponId::Mp7:
                    case WeaponId::Ak47:
                    case WeaponId::G3SG1:
                    case WeaponId::Sawedoff:
                        return 3;
                    case WeaponId::Elite:
                    case WeaponId::Mp5sd:
                    case WeaponId::M4A1:
                    case WeaponId::Scar20:
                    case WeaponId::Mag7:
                        return 4;
                    case WeaponId::P250:
                    case WeaponId::Ump45:
                    case WeaponId::M4a1_s:
                        return 5;
                    case WeaponId::Tec9:
                    case WeaponId::P90:
                    case WeaponId::Sg553:
                        return 6;
                    case WeaponId::Fiveseven:
                    case WeaponId::Bizon:
                    case WeaponId::Aug:
                        return 7;
                    case WeaponId::Cz75a: return 8;
                    case WeaponId::Deagle: return 9;
                    case WeaponId::Revolver: return 10;
                    }
                };

                if (const auto weaponData = entity->getWeaponData(); weaponData && !config.weapons.enabled) {
                    switch (weaponData->type) {
                    case WeaponType::Pistol:
                        renderWeaponEsp(drawList, entity, config.pistols[0], config.pistols[getWeaponIndex(entity->weaponId())]);
                        break;
                    case WeaponType::SubMachinegun:
                        renderWeaponEsp(drawList, entity, config.smgs[0], config.smgs[getWeaponIndex(entity->weaponId())]);
                        break;
                    case WeaponType::Rifle:
                        renderWeaponEsp(drawList, entity, config.rifles[0], config.rifles[getWeaponIndex(entity->weaponId())]);
                        break;
                    case WeaponType::SniperRifle:
                        renderWeaponEsp(drawList, entity, config.sniperRifles[0], config.sniperRifles[getWeaponIndex(entity->weaponId())]);
                        break;
                    case WeaponType::Shotgun:
                        renderWeaponEsp(drawList, entity, config.shotguns[0], config.shotguns[getWeaponIndex(entity->weaponId())]);
                        break;
                    case WeaponType::Machinegun:
                        renderWeaponEsp(drawList, entity, config.heavy[0], config.heavy[getWeaponIndex(entity->weaponId())]);
                        break;
                    }
                } else {
                    renderWeaponEsp(drawList, entity, config.weapons, config.weapons);
                }
            } else {
                switch (entity->getClientClass()->classId) {
                case ClassId::EconEntity:
                    renderEntityEsp(drawList, entity, config.misc[0], config.misc[1], nullptr, interfaces.localize->find("#SFUI_WPNHUD_DEFUSER"));
                    break;
                case ClassId::Chicken:
                    renderEntityEsp(drawList, entity, config.misc[0], config.misc[2], "Chicken");
                    break;
                case ClassId::PlantedC4:
                    renderEntityEsp(drawList, entity, config.misc[0], config.misc[3], "Planted C4");
                    break;
                }
            }
        }
    }
}
