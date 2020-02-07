#define NOMINMAX
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
#include "../SDK/WeaponInfo.h"
#include "../SDK/WeaponId.h"

#include <mutex>
#include <optional>
#include <tuple>

static D3DMATRIX viewMatrix;

static bool worldToScreen(const Vector& in, ImVec2& out) noexcept
{
    const auto& matrix = viewMatrix;

    float w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;

    if (w > 0.001f) {
        const auto [width, height] = interfaces->engine->getScreenSize();
        out.x = width / 2 * (1 + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w);
        out.y = height / 2 * (1 - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w);
        return true;
    }
    return false;
}

struct BoundingBox {
    ImVec2 min, max;
    ImVec2 vertices[8];
};

struct BaseData {
    Vector absOrigin;
    Matrix3x4 coordinateFrame;
    Vector obbMins;
    Vector obbMaxs;
    float distanceToLocal;
};

struct EntityData : BaseData {
    ClassId classId;
    bool flashbang;
};

struct PlayerData : BaseData {
    bool enemy;
    bool visible;
    std::string name;
    std::string activeWeapon;
};

struct WeaponData : BaseData {
    std::string name;
    WeaponType type = WeaponType::Unknown;
    WeaponId id;
    int clip;
    int reserveAmmo;
};

static std::vector<PlayerData> players;
static std::vector<WeaponData> weapons;
static std::vector<EntityData> entities;
static std::mutex dataMutex;

void ESP::collectData() noexcept
{
    std::scoped_lock _{ dataMutex };

    players.clear();
    weapons.clear();
    entities.clear();

    const auto localPlayer = interfaces->entityList->getEntity(interfaces->engine->getLocalPlayer());

    if (!localPlayer)
        return;

    viewMatrix = interfaces->engine->worldToScreenMatrix();

    const auto observerTarget = localPlayer->getObserverTarget();

    for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer || entity == observerTarget
            || entity->isDormant() || !entity->isAlive())
            continue;

        PlayerData data;
        data.absOrigin = entity->getAbsOrigin();
        data.coordinateFrame = entity->coordinateFrame();
        data.obbMins = entity->getCollideable()->obbMins();
        data.obbMaxs = entity->getCollideable()->obbMaxs();
        data.distanceToLocal = (localPlayer->getAbsOrigin() - entity->getAbsOrigin()).length();

        data.enemy = memory->isOtherEnemy(entity, localPlayer);
        data.visible = entity->visibleTo(localPlayer);

        if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(entity->index(), playerInfo)) {
            if (config->normalizePlayerNames) {
                if (wchar_t nameWide[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, 128, nameWide, 128)) {
                    if (wchar_t nameNormalized[128]; NormalizeString(NormalizationKC, nameWide, -1, nameNormalized, 128)) {
                        if (WideCharToMultiByte(CP_UTF8, 0, nameNormalized, -1, playerInfo.name, 128, nullptr, nullptr))
                            data.name = playerInfo.name;
                    }
                }
            } else {
                data.name = playerInfo.name;
            }
        }
        if (const auto weapon = entity->getActiveWeapon()) {
            if (const auto weaponData = weapon->getWeaponInfo()) {
                if (char weaponName[100]; WideCharToMultiByte(CP_UTF8, 0, interfaces->localize->find(weaponData->name), -1, weaponName, _countof(weaponName), nullptr, nullptr))
                    data.activeWeapon = weaponName;
            }
        }
        players.push_back(data);
    }

    for (int i = interfaces->engine->getMaxClients() + 1; i <= interfaces->entityList->getHighestEntityIndex(); ++i) {
        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity->isDormant())
            continue;

        if (entity->isWeapon()) {
            if (entity->ownerEntity() == -1) {
                WeaponData data;
                data.absOrigin = entity->getAbsOrigin();
                data.coordinateFrame = entity->coordinateFrame();
                data.obbMins = entity->getCollideable()->obbMins();
                data.obbMaxs = entity->getCollideable()->obbMaxs();
                data.distanceToLocal = (localPlayer->getAbsOrigin() - entity->getAbsOrigin()).length();

                if (const auto weaponData = entity->getWeaponInfo()) {
                    data.name = weaponData->name;
                    data.type = weaponData->type;
                }
                data.id = entity->weaponId();
                data.clip = entity->clip();
                data.reserveAmmo = entity->reserveAmmoCount();
                weapons.push_back(data);
            }
        } else {
            const auto classId = entity->getClientClass()->classId;

            switch (classId) {
            case ClassId::BaseCSGrenadeProjectile:
            case ClassId::BreachChargeProjectile:
            case ClassId::BumpMineProjectile:
            case ClassId::DecoyProjectile:
            case ClassId::MolotovProjectile:
            case ClassId::SensorGrenadeProjectile:
            case ClassId::SmokeGrenadeProjectile:
            case ClassId::SnowballProjectile:

            case ClassId::EconEntity:
            case ClassId::Chicken:
            case ClassId::PlantedC4:
                EntityData data;
                data.absOrigin = entity->getAbsOrigin();
                data.coordinateFrame = entity->coordinateFrame();
                data.obbMins = entity->getCollideable()->obbMins();
                data.obbMaxs = entity->getCollideable()->obbMaxs();
                data.distanceToLocal = (localPlayer->getAbsOrigin() - entity->getAbsOrigin()).length();
                data.classId = classId;

                if (const auto model = entity->getModel(); model && std::strstr(model->name, "flashbang"))
                    data.flashbang = true;
                else
                    data.flashbang = false;

                entities.push_back(data);
            }
        }
    }
}

static auto boundingBox(const BaseData& entityData, BoundingBox& out) noexcept
{
    const auto [width, height] = interfaces->engine->getScreenSize();

    out.min.x = static_cast<float>(width * 2);
    out.min.y = static_cast<float>(height * 2);
    out.max.x = -out.min.x;
    out.max.y = -out.min.y;

    const auto mins = entityData.obbMins;
    const auto maxs = entityData.obbMaxs;

    for (int i = 0; i < 8; ++i) {
        const Vector point{ i & 1 ? maxs.x : mins.x,
                            i & 2 ? maxs.y : mins.y,
                            i & 4 ? maxs.z : mins.z };

        if (!worldToScreen(point.transform(entityData.coordinateFrame), out.vertices[i]))
            return false;

        out.min.x = std::min(out.min.x, out.vertices[i].x);
        out.min.y = std::min(out.min.y, out.vertices[i].y);
        out.max.x = std::max(out.max.x, out.vertices[i].x);
        out.max.y = std::max(out.max.y, out.vertices[i].y);
    }
    return true;
}

static void renderBox(ImDrawList* drawList, const BoundingBox& bbox, const Config::Shared& config) noexcept
{
    if (!config.box.enabled)
        return;

    const ImU32 color = Helpers::calculateColor(config.box.color, config.box.rainbow, config.box.rainbowSpeed, memory->globalVars->realtime);

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

static void renderText(ImDrawList* drawList, float distance, float cullDistance, const Config::Color& textCfg, const Config::ColorToggleRounding& backgroundCfg, const char* text, const ImVec2& pos, bool centered = true, bool adjustHeight = true) noexcept
{
    if (cullDistance && Helpers::units2meters(distance) > cullDistance)
        return;

    const auto fontSize = std::clamp(15.0f * 10.0f / std::sqrt(distance), 10.0f, 15.0f);

    const auto textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, -1.0f, text);
    const auto horizontalOffset = centered ? textSize.x / 2 : 0.0f;
    const auto verticalOffset = adjustHeight ? textSize.y : 0.0f;

    if (backgroundCfg.enabled) {
        const ImU32 color = Helpers::calculateColor(backgroundCfg.color, backgroundCfg.rainbow, backgroundCfg.rainbowSpeed, memory->globalVars->realtime);
        drawList->AddRectFilled({ pos.x - horizontalOffset - 2, pos.y - verticalOffset - 2 }, { pos.x - horizontalOffset + textSize.x + 2, pos.y - verticalOffset + textSize.y + 2 }, color, backgroundCfg.rounding);
    }
    const ImU32 color = Helpers::calculateColor(textCfg.color, textCfg.rainbow, textCfg.rainbowSpeed, memory->globalVars->realtime);
    drawList->AddText(nullptr, fontSize, { pos.x - horizontalOffset, pos.y - verticalOffset }, color, text);
}

static void renderSnaplines(ImDrawList* drawList, const BoundingBox& bbox, const Config::ColorToggleThickness& config, int type) noexcept
{
    if (!config.enabled)
        return;

    const auto [width, height] = interfaces->engine->getScreenSize();
    const ImU32 color = Helpers::calculateColor(config.color, config.rainbow, config.rainbowSpeed, memory->globalVars->realtime);
    drawList->AddLine({ static_cast<float>(width / 2), static_cast<float>(type ? 0 : height) }, { (bbox.min.x + bbox.max.x) / 2, type ? bbox.min.y : bbox.max.y }, color, config.thickness);
}

static void renderPlayerBox(ImDrawList* drawList, const PlayerData& playerData, const Config::Player& config) noexcept
{
    BoundingBox bbox;

    if (!boundingBox(playerData, bbox))
        return;

    renderBox(drawList, bbox, config);
    renderSnaplines(drawList, bbox, config.snaplines, config.snaplineType);

    ImGui::PushFont(::config->fonts[config.font]);

    if (config.name.enabled && !playerData.name.empty())
        renderText(drawList, playerData.distanceToLocal, config.textCullDistance, config.name, config.textBackground, playerData.name.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });

    if (config.weapon.enabled && !playerData.activeWeapon.empty())
        renderText(drawList, playerData.distanceToLocal, config.textCullDistance, config.weapon, config.textBackground, playerData.activeWeapon.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);

    ImGui::PopFont();
}

static void renderWeaponBox(ImDrawList* drawList, const WeaponData& weaponData, const Config::Weapon& config) noexcept
{
    if (BoundingBox bbox; boundingBox(weaponData, bbox)) {
        renderBox(drawList, bbox, config);
        renderSnaplines(drawList, bbox, config.snaplines, config.snaplineType);

        ImGui::PushFont(::config->fonts[config.font]);

        if (config.name.enabled && !weaponData.name.empty()) {
            if (char weaponName[100]; WideCharToMultiByte(CP_UTF8, 0, interfaces->localize->find(weaponData.name.c_str()), -1, weaponName, _countof(weaponName), nullptr, nullptr))
                renderText(drawList, weaponData.distanceToLocal, config.textCullDistance, config.name, config.textBackground, weaponName, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
        }

        if (config.ammo.enabled && weaponData.clip != -1) {
            const auto text{ std::to_string(weaponData.clip) + " / " + std::to_string(weaponData.reserveAmmo) };
            renderText(drawList, weaponData.distanceToLocal, config.textCullDistance, config.ammo, config.textBackground, text.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);
        }

        ImGui::PopFont();
    }
}

static void renderEntityBox(ImDrawList* drawList, const EntityData& entityData, const char* name, const Config::Shared& config) noexcept
{
    if (BoundingBox bbox; boundingBox(entityData, bbox)) {
        renderBox(drawList, bbox, config);
        renderSnaplines(drawList, bbox, config.snaplines, config.snaplineType);

        ImGui::PushFont(::config->fonts[config.font]);

        if (config.name.enabled)
            renderText(drawList, entityData.distanceToLocal, config.textCullDistance, config.name, config.textBackground, name, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });

        ImGui::PopFont();
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

static constexpr bool renderPlayerEsp(ImDrawList* drawList, const PlayerData& playerData, EspId id) noexcept
{
    if (config->players[id].enabled) {
        renderPlayerBox(drawList, playerData, config->players[id]);
    }
    return config->players[id].enabled;
}

static constexpr void renderWeaponEsp(ImDrawList* drawList, const WeaponData& weaponData, const Config::Weapon& parentConfig, const Config::Weapon& itemConfig) noexcept
{
    const auto& config = parentConfig.enabled ? parentConfig : itemConfig;
    if (config.enabled) {
        renderWeaponBox(drawList, weaponData, config);
    }
}

static void renderEntityEsp(ImDrawList* drawList, const EntityData& entityData, const Config::Shared& parentConfig, const Config::Shared& itemConfig, const char* name) noexcept
{
    const auto& config = parentConfig.enabled ? parentConfig : itemConfig;

    if (config.enabled) {
        renderEntityBox(drawList, entityData, name, config);
    }
}

void ESP::render(ImDrawList* drawList) noexcept
{
    std::scoped_lock _{ dataMutex };

    for (const auto& player : players) {
        if (!player.enemy) {
            if (!renderPlayerEsp(drawList, player, ALLIES_ALL)) {
                if (player.visible)
                    renderPlayerEsp(drawList, player, ALLIES_VISIBLE);
                else
                    renderPlayerEsp(drawList, player, ALLIES_OCCLUDED);
            }
        } else if (!renderPlayerEsp(drawList, player, ENEMIES_ALL)) {
            if (player.visible)
                renderPlayerEsp(drawList, player, ENEMIES_VISIBLE);
            else
                renderPlayerEsp(drawList, player, ENEMIES_OCCLUDED);
        }
    }

    for (const auto& weapon : weapons) {
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

        if (!config->weapons.enabled) {
            constexpr auto dispatchWeapon = [](WeaponType type, int idx) -> std::optional<std::pair<const Config::Weapon&, const Config::Weapon&>> {
                switch (type) {
                case WeaponType::Pistol: return { { config->pistols[0], config->pistols[idx] } };
                case WeaponType::SubMachinegun: return { { config->smgs[0], config->smgs[idx] } };
                case WeaponType::Rifle: return { { config->rifles[0], config->rifles[idx] } };
                case WeaponType::SniperRifle: return { { config->sniperRifles[0], config->sniperRifles[idx] } };
                case WeaponType::Shotgun: return { { config->shotguns[0], config->shotguns[idx] } };
                case WeaponType::Machinegun: return { { config->machineguns[0], config->machineguns[idx] } };
                case WeaponType::Grenade: return { { config->grenades[0], config->grenades[idx] } };
                default: return std::nullopt;
                }
            };

            if (const auto w = dispatchWeapon(weapon.type, getWeaponIndex(weapon.id)))
                renderWeaponEsp(drawList, weapon, w->first, w->second);
        } else {
            renderWeaponEsp(drawList, weapon, config->weapons, config->weapons);
        }
    }

    for (const auto& entity : entities) {
        constexpr auto dispatchEntity = [](ClassId classId, bool flashbang) -> std::optional<std::tuple<const Config::Shared&, const Config::Shared&, const char*>> {
            switch (classId) {
            case ClassId::BaseCSGrenadeProjectile:
                if (flashbang) return  { { config->projectiles[0], config->projectiles[1], "Flashbang" } };
                else return  { { config->projectiles[0], config->projectiles[2], "HE Grenade" } };
            case ClassId::BreachChargeProjectile: return { { config->projectiles[0], config->projectiles[3], "Breach Charge" } };
            case ClassId::BumpMineProjectile: return { { config->projectiles[0], config->projectiles[4], "Bump Mine" } };
            case ClassId::DecoyProjectile: return  { { config->projectiles[0], config->projectiles[5], "Decoy Grenade" } };
            case ClassId::MolotovProjectile: return { { config->projectiles[0], config->projectiles[6], "Molotov" } };
            case ClassId::SensorGrenadeProjectile: return { { config->projectiles[0], config->projectiles[7], "TA Grenade" } };
            case ClassId::SmokeGrenadeProjectile: return { { config->projectiles[0], config->projectiles[8], "Smoke Grenade" } };
            case ClassId::SnowballProjectile: return { { config->projectiles[0], config->projectiles[9], "Snowball" } };

            case ClassId::EconEntity: return { { config->otherEntities[0], config->otherEntities[1], "Defuse Kit" } };
            case ClassId::Chicken: return { { config->otherEntities[0], config->otherEntities[2], "Chicken" } };
            case ClassId::PlantedC4: return { { config->otherEntities[0], config->otherEntities[3], "Planted C4" } };
            default: return std::nullopt;
            }
        };

        if (const auto e = dispatchEntity(entity.classId, entity.flashbang))
            renderEntityEsp(drawList, entity, std::get<0>(*e), std::get<1>(*e), std::get<2>(*e));
    }
}
