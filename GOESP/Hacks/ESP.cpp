#include "ESP.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

#include "../Config.h"
#include "../Helpers.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../SDK/ClassId.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Localize.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponData.h"
#include "../SDK/WeaponId.h"

#include <optional>
#include <tuple>

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
        const ImU32 color = Helpers::calculateColor(config.box.color, config.box.rainbow, config.box.rainbowSpeed, memory.globalVars->realtime);

        switch (config.boxType) {
        case 0:
            drawList->AddRect(bbox.min, bbox.max, color, config.box.rounding, ImDrawCornerFlags_All, config.box.thickness);
            break;

        case 1:
            drawList->AddLine(bbox.min, { bbox.min.x, bbox.min.y + (bbox.max.y - bbox.min.y) * 0.25f }, color, config.box.thickness);
            drawList->AddLine(bbox.min, { bbox.min.x + (bbox.max.x - bbox.min.x) * 0.25f, bbox.min.y }, color, config.box.thickness);

            drawList->AddLine({ bbox.max.x, bbox.min.y }, { bbox.max.x - (bbox.max.x - bbox.min.x) * 0.25f, bbox.min.y }, color, config.box.thickness);
            drawList->AddLine({ bbox.max.x, bbox.min.y }, { bbox.max.x, bbox.min.y + (bbox.max.y - bbox.min.y) * 0.25f }, color, config.box.thickness);

            drawList->AddLine({ bbox.min.x, bbox.max.y }, { bbox.min.x, bbox.max.y - (bbox.max.y - bbox.min.y) * 0.25f }, color, config.box.thickness);
            drawList->AddLine({ bbox.min.x, bbox.max.y }, { bbox.min.x + (bbox.max.x - bbox.min.x) * 0.25f, bbox.max.y }, color, config.box.thickness);

            drawList->AddLine(bbox.max, { bbox.max.x - (bbox.max.x - bbox.min.x) * 0.25f, bbox.max.y }, color, config.box.thickness);
            drawList->AddLine(bbox.max, { bbox.max.x, bbox.max.y - (bbox.max.y - bbox.min.y) * 0.25f }, color, config.box.thickness);
            break;
        case 2:
            for (int i = 0; i < 8; ++i) {
                for (int j = 1; j <= 4; j <<= 1) {
                    if (!(i & j))
                        drawList->AddLine(bbox.vertices[i], bbox.vertices[i + j], color, config.box.thickness);
                }
            }
            break;
        case 3:
            for (int i = 0; i < 8; ++i) {
                for (int j = 1; j <= 4; j <<= 1) {
                    if (!(i & j)) {
                        drawList->AddLine(bbox.vertices[i], { bbox.vertices[i].x + (bbox.vertices[i + j].x - bbox.vertices[i].x) * 0.25f, bbox.vertices[i].y + (bbox.vertices[i + j].y - bbox.vertices[i].y) * 0.25f }, color, config.box.thickness);
                        drawList->AddLine({ bbox.vertices[i].x + (bbox.vertices[i + j].x - bbox.vertices[i].x) * 0.75f, bbox.vertices[i].y + (bbox.vertices[i + j].y - bbox.vertices[i].y) * 0.75f }, bbox.vertices[i + j], color, config.box.thickness);
                    }
                }
            }
            break;
        }
    }
}

static void renderText(ImDrawList* drawList, Entity* entity, const Config::Color& textCfg, const Config::ColorToggleRounding& backgroundCfg, const char* text, const ImVec2& pos, bool centered = true, bool adjustHeight = true) noexcept
{
    const auto distance = (interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer())->getAbsOrigin() - entity->getAbsOrigin()).length();
    const auto fontSize = std::clamp(15.0f * 10.0f / std::sqrt(distance), 10.0f, 15.0f);

    ImGui::GetCurrentContext()->FontSize = fontSize;
    const auto textSize = ImGui::CalcTextSize(text);
    const auto horizontalOffset = centered ? textSize.x / 2 : 0.0f;
    const auto verticalOffset = adjustHeight ? textSize.y : 0.0f;

    if (backgroundCfg.enabled) {
        const ImU32 color = Helpers::calculateColor(backgroundCfg.color, backgroundCfg.rainbow, backgroundCfg.rainbowSpeed, memory.globalVars->realtime);
        drawList->AddRectFilled({ pos.x - horizontalOffset - 2, pos.y - verticalOffset - 2 }, { pos.x - horizontalOffset + textSize.x + 2, pos.y - verticalOffset + textSize.y + 2 }, color, backgroundCfg.rounding);
    }
    const ImU32 color = Helpers::calculateColor(textCfg.color, textCfg.rainbow, textCfg.rainbowSpeed, memory.globalVars->realtime);
    drawList->AddText(nullptr, fontSize, { pos.x - horizontalOffset, pos.y - verticalOffset }, color, text);
}

static void renderPlayerBox(ImDrawList* drawList, Entity* entity, const Config::Player& config) noexcept
{
    if (BoundingBox bbox; boundingBox(entity, bbox)) {
        renderBox(drawList, entity, bbox, config);

        ImGui::PushFont(::config.fonts[config.font]);
        const auto oldFontSize = ImGui::GetCurrentContext()->FontSize;

        if (config.name.enabled) {
            if (PlayerInfo playerInfo; interfaces.engine->getPlayerInfo(entity->index(), playerInfo))
                renderText(drawList, entity, config.name, config.textBackground, playerInfo.name, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
        }

        if (config.weapon.enabled) {
            if (const auto weapon = entity->getActiveWeapon()) {
                if (const auto weaponData = weapon->getWeaponData()) {
                    if (char weaponName[100]; WideCharToMultiByte(CP_UTF8, 0, interfaces.localize->find(weaponData->name), -1, weaponName, _countof(weaponName), nullptr, nullptr))
                        renderText(drawList, entity, config.weapon, config.textBackground, weaponName, { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);
                }
            }
        }

        ImGui::GetCurrentContext()->FontSize = oldFontSize;
        ImGui::PopFont();
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
                if (char weaponName[100]; WideCharToMultiByte(CP_UTF8, 0, interfaces.localize->find(weaponData->name), -1, weaponName, _countof(weaponName), nullptr, nullptr))
                    renderText(drawList, entity, config.name, config.textBackground, weaponName, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
            }
        }

        if (config.ammo.enabled && entity->clip() != -1) {
            const auto text{ std::to_string(entity->clip()) + " / " + std::to_string(entity->reserveAmmoCount()) };
            renderText(drawList, entity, config.ammo, config.textBackground, text.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);
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

        if (config.name.enabled)
            renderText(drawList, entity, config.name, config.textBackground, name, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });

        ImGui::GetCurrentContext()->FontSize = oldFontSize;
        ImGui::PopFont();
    }
}

static void renderSnaplines(ImDrawList* drawList, Entity* entity, const Config::ColorToggleThickness& config) noexcept
{
    if (config.enabled) {
        if (ImVec2 position; worldToScreen(entity->getAbsOrigin(), position)) {
            const auto [width, height] = interfaces.engine->getScreenSize();
            const ImU32 color = Helpers::calculateColor(config.color, config.rainbow, config.rainbowSpeed, memory.globalVars->realtime);
            drawList->AddLine({ static_cast<float>(width / 2), static_cast<float>(height) }, position, color, config.thickness);
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

static void renderEntityEsp(ImDrawList* drawList, Entity* entity, const Config::Shared& parentConfig, const Config::Shared& itemConfig, const char* name) noexcept
{
    const auto& config = parentConfig.enabled ? parentConfig : itemConfig;

    if (config.enabled) {
        renderEntityBox(drawList, entity, name, config);
        renderSnaplines(drawList, entity, config.snaplines);
    }
}

static void renderEntityEsp(ImDrawList* drawList, Entity* entity, const Config::Shared& parentConfig, const Config::Shared& itemConfig, const wchar_t* name) noexcept
{
    if (char nameConverted[100]; WideCharToMultiByte(CP_UTF8, 0, name, -1, nameConverted, _countof(nameConverted), nullptr, nullptr))
        renderEntityEsp(drawList, entity, parentConfig, itemConfig, nameConverted);
}

void ESP::render(ImDrawList* drawList) noexcept
{
    if (interfaces.engine->isInGame()) {
        const auto localPlayer = interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer());

        if (!localPlayer)
            return;

        const auto observerTarget = localPlayer->getObserverTarget();

        for (int i = 1; i <= interfaces.engine->getMaxClients(); ++i) {
            const auto entity = interfaces.entityList->getEntity(i);
            if (!entity || entity == localPlayer || entity == observerTarget
                || entity->isDormant() || !entity->isAlive())
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
                    case WeaponId::Flashbang:
                        return 1;
                    case WeaponId::Hkp2000:
                    case WeaponId::Mp9:
                    case WeaponId::Famas:
                    case WeaponId::Awp:
                    case WeaponId::Xm1014:
                    case WeaponId::Negev:
                    case WeaponId::HeGrenade:
                        return 2;
                    case WeaponId::Usp_s:
                    case WeaponId::Mp7:
                    case WeaponId::Ak47:
                    case WeaponId::G3SG1:
                    case WeaponId::Sawedoff:
                    case WeaponId::SmokeGrenade:
                        return 3;
                    case WeaponId::Elite:
                    case WeaponId::Mp5sd:
                    case WeaponId::M4A1:
                    case WeaponId::Scar20:
                    case WeaponId::Mag7:
                    case WeaponId::Molotov:
                        return 4;
                    case WeaponId::P250:
                    case WeaponId::Ump45:
                    case WeaponId::M4a1_s:
                    case WeaponId::Decoy:
                        return 5;
                    case WeaponId::Tec9:
                    case WeaponId::P90:
                    case WeaponId::Sg553:
                    case WeaponId::IncGrenade:
                        return 6;
                    case WeaponId::Fiveseven:
                    case WeaponId::Bizon:
                    case WeaponId::Aug:
                    case WeaponId::TaGrenade:
                        return 7;
                    case WeaponId::Cz75a:
                    case WeaponId::Firebomb:
                        return 8;
                    case WeaponId::Deagle:
                    case WeaponId::Diversion:
                        return 9;
                    case WeaponId::Revolver:
                    case WeaponId::FragGrenade:
                        return 10;
                    case WeaponId::Snowball:
                        return 11;
                    }
                };

                if (const auto weaponData = entity->getWeaponData(); weaponData && !config.weapons.enabled) {
                    constexpr auto dispatchWeapon = [](WeaponType type, int idx) -> std::optional<std::pair<const Config::Weapon&, const Config::Weapon&>> {
                        switch (type) {
                        case WeaponType::Pistol: return { { config.pistols[0], config.pistols[idx] } };
                        case WeaponType::SubMachinegun: return { { config.smgs[0], config.smgs[idx] } };
                        case WeaponType::Rifle: return { { config.rifles[0], config.rifles[idx] } };
                        case WeaponType::SniperRifle: return { { config.sniperRifles[0], config.sniperRifles[idx] } };
                        case WeaponType::Shotgun: return { { config.shotguns[0], config.shotguns[idx] } };
                        case WeaponType::Machinegun: return { { config.machineguns[0], config.machineguns[idx] } };
                        case WeaponType::Grenade: return { { config.grenades[0], config.grenades[idx] } };
                        default: return std::nullopt;
                        }
                    };

                    if (const auto w = dispatchWeapon(weaponData->type, getWeaponIndex(entity->weaponId())))
                        renderWeaponEsp(drawList, entity, w->first, w->second);
                } else {
                    renderWeaponEsp(drawList, entity, config.weapons, config.weapons);
                }
            } else {
                constexpr auto dispatchEntity = [](ClassId classId, bool flashbang) -> std::optional<std::tuple<const Config::Shared&, const Config::Shared&, const char*>> {
                    switch (classId) {
                    case ClassId::BaseCSGrenadeProjectile:
                        if (flashbang) return  { { config.projectiles[0], config.projectiles[1], "Flashbang" } };
                        else return  { { config.projectiles[0], config.projectiles[2], "HE Grenade" } };
                    case ClassId::BreachChargeProjectile: return { { config.projectiles[0], config.projectiles[3], "Breach Charge" } };
                    case ClassId::BumpMineProjectile: return { { config.projectiles[0], config.projectiles[4], "Bump Mine" } };
                    case ClassId::DecoyProjectile: return  { { config.projectiles[0], config.projectiles[5], "Decoy Grenade" } };
                    case ClassId::MolotovProjectile: return { { config.projectiles[0], config.projectiles[6], "Molotov" } };
                    case ClassId::SensorGrenadeProjectile: return { { config.projectiles[0], config.projectiles[7], "TA Grenade" } };
                    case ClassId::SmokeGrenadeProjectile: return { { config.projectiles[0], config.projectiles[8], "Smoke Grenade" } };
                    case ClassId::SnowballProjectile: return { { config.projectiles[0], config.projectiles[9], "Snowball" } };

                    case ClassId::EconEntity: return { { config.otherEntities[0], config.otherEntities[1], "Defuse Kit" } };
                    case ClassId::Chicken: return { { config.otherEntities[0], config.otherEntities[2], "Chicken" } };
                    case ClassId::PlantedC4: return { { config.otherEntities[0], config.otherEntities[3], "Planted C4" } };
                    default: return std::nullopt;
                    }
                };

                if (const auto e = dispatchEntity(entity->getClientClass()->classId, entity->getModel() && std::strstr(entity->getModel()->name, "flashbang")))
                    renderEntityEsp(drawList, entity, std::get<0>(*e), std::get<1>(*e), std::get<2>(*e));
            }
        }
    }
}
