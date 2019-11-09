#include "ESP.h"

#include "imgui/imgui.h"

#include "Config.h"
#include "Interfaces.h"
#include "Memory.h"
#include "SDK/ClassId.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/GlobalVars.h"
#include "SDK/Utils.h"
#include "SDK/Vector.h"
#include "SDK/WeaponId.h"

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

    for (int i = 0; i < 8; i++) {
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
        ImU32 color = config.box.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.box.rainbowSpeed)) : ImGui::ColorConvertFloat4ToU32(config.box.color);

        switch (config.boxType) {
        case 0:
            drawList->AddRect(bbox.min, bbox.max, color);
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
            for (int i = 0; i < 8; i++) {
                for (int j = 1; j <= 4; j <<= 1) {
                    if (!(i & j))
                        drawList->AddLine(bbox.vertices[i], bbox.vertices[i + j], color);
                }
            }
            break;
        case 3:
            for (int i = 0; i < 8; i++) {
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
    }
}

static void renderWeaponBox(ImDrawList* drawList, Entity* entity, const Config::Weapon& config) noexcept
{
    if (BoundingBox bbox; boundingBox(entity, bbox)) {
        renderBox(drawList, entity, bbox, config);
    }
}

static void renderSnaplines(ImDrawList* drawList, Entity* entity, const Config::ColorToggle& config) noexcept
{
    if (config.enabled) {
        if (ImVec2 position; worldToScreen(entity->getAbsOrigin(), position)) {
            const auto [width, height] = interfaces.engine->getScreenSize();
            drawList->AddLine({ static_cast<float>(width / 2), static_cast<float>(height) }, position, config.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(memory.globalVars->realtime, config.rainbowSpeed)) : ImGui::ColorConvertFloat4ToU32(config.color));
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

static constexpr bool renderWeaponEsp(ImDrawList* drawList, Entity* entity, const Config::Weapon& config) noexcept
{
    if (config.enabled) {
        renderWeaponBox(drawList, entity, config);
        renderSnaplines(drawList, entity, config.snaplines);
    }
    return config.enabled;
}

void ESP::render(ImDrawList* drawList) noexcept
{
    if (interfaces.engine->isInGame()) {
        const auto localPlayer = interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer());

        if (!localPlayer)
            return;

        for (int i = 1; i <= interfaces.engine->getMaxClients(); i++) {
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

        for (int i = interfaces.engine->getMaxClients() + 1; i <= interfaces.entityList->getHighestEntityIndex(); i++) {
            const auto entity = interfaces.entityList->getEntity(i);
            if (!entity || entity->isDormant())
                continue;

            if (entity->isWeapon() && entity->ownerEntity() == -1 && !renderWeaponEsp(drawList, entity, config.weapons)) {
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

                switch (entity->weaponId()) {
                case WeaponId::Glock:
                case WeaponId::Hkp2000:
                case WeaponId::Usp_s:
                case WeaponId::Elite:
                case WeaponId::P250:
                case WeaponId::Tec9:
                case WeaponId::Fiveseven:
                case WeaponId::Cz75a:
                case WeaponId::Deagle:
                case WeaponId::Revolver:
                    if (!renderWeaponEsp(drawList, entity, config.pistols[0]))
                        renderWeaponEsp(drawList, entity, config.pistols[getWeaponIndex(entity->weaponId())]);
                    break;
                case WeaponId::Mac10:
                case WeaponId::Mp9:
                case WeaponId::Mp7:
                case WeaponId::Mp5sd:
                case WeaponId::Ump45:
                case WeaponId::P90:
                case WeaponId::Bizon:
                    if (!renderWeaponEsp(drawList, entity, config.smgs[0]))
                        renderWeaponEsp(drawList, entity, config.smgs[getWeaponIndex(entity->weaponId())]);
                    break;
                case WeaponId::GalilAr:
                case WeaponId::Famas:
                case WeaponId::Ak47:
                case WeaponId::M4A1:
                case WeaponId::M4a1_s:
                case WeaponId::Sg553:
                case WeaponId::Aug:
                    if (!renderWeaponEsp(drawList, entity, config.rifles[0]))
                        renderWeaponEsp(drawList, entity, config.rifles[getWeaponIndex(entity->weaponId())]);
                    break;
                case WeaponId::Ssg08:
                case WeaponId::Awp:
                case WeaponId::G3SG1:
                case WeaponId::Scar20:
                    if (!renderWeaponEsp(drawList, entity, config.sniperRifles[0]))
                        renderWeaponEsp(drawList, entity, config.sniperRifles[getWeaponIndex(entity->weaponId())]);
                    break;
                case WeaponId::Nova:
                case WeaponId::Xm1014:
                case WeaponId::Sawedoff:
                case WeaponId::Mag7:
                    if (!renderWeaponEsp(drawList, entity, config.shotguns[0]))
                        renderWeaponEsp(drawList, entity, config.shotguns[getWeaponIndex(entity->weaponId())]);
                    break;
                case WeaponId::M249:
                case WeaponId::Negev:
                    if (!renderWeaponEsp(drawList, entity, config.heavy[0]))
                        renderWeaponEsp(drawList, entity, config.heavy[getWeaponIndex(entity->weaponId())]);
                    break;
                }
            } else {
                switch (entity->getClientClass()->classId) {
                case ClassId::EconEntity:
                    if (!renderWeaponEsp(drawList, entity, config.misc[0]))
                        renderWeaponEsp(drawList, entity, config.misc[1]);
                    break;
                case ClassId::Chicken:
                    if (!renderWeaponEsp(drawList, entity, config.misc[0]))
                        renderWeaponEsp(drawList, entity, config.misc[2]);
                    break;
                }
            }
        }
    }
}
