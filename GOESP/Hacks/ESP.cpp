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

struct BaseData {
    BaseData(Entity* entity) noexcept
    {
        const auto localPlayer = interfaces->entityList->getEntity(interfaces->engine->getLocalPlayer());

        if (!localPlayer)
            return;

        distanceToLocal = (localPlayer->getAbsOrigin() - entity->getAbsOrigin()).length();
        obbMins = entity->getCollideable()->obbMins();
        obbMaxs = entity->getCollideable()->obbMaxs();
        coordinateFrame = entity->toWorldTransform();
    }
    float distanceToLocal;
    Vector obbMins;
    Vector obbMaxs;
    Matrix3x4 coordinateFrame;
};

struct EntityData : BaseData {
    EntityData(Entity* entity) noexcept : BaseData{ entity }
    {
        classId = entity->getClientClass()->classId;

        if (const auto model = entity->getModel(); model && std::strstr(model->name, "flashbang"))
            flashbang = true;
        else
            flashbang = false;
    }
    ClassId classId;
    bool flashbang;
};

struct PlayerData : BaseData {
    PlayerData(Entity* entity) noexcept : BaseData{ entity }
    {
        const auto localPlayer = interfaces->entityList->getEntity(interfaces->engine->getLocalPlayer());

        if (!localPlayer)
            return;

        enemy = memory->isOtherEnemy(entity, localPlayer);
        visible = entity->visibleTo(localPlayer);
        flashDuration = entity->flashDuration();

        name = entity->getPlayerName(config->normalizePlayerNames);

        if (const auto weapon = entity->getActiveWeapon()) {
            if (const auto weaponData = weapon->getWeaponInfo()) {
                if (char weaponName[100]; WideCharToMultiByte(CP_UTF8, 0, interfaces->localize->find(weaponData->name), -1, weaponName, _countof(weaponName), nullptr, nullptr))
                    activeWeapon = weaponName;
            }
        }
    }
    bool enemy;
    bool visible;
    float flashDuration;
    std::string name;
    std::string activeWeapon;
};

struct WeaponData : BaseData {
    WeaponData(Entity* entity) noexcept : BaseData{ entity }
    {
        clip = entity->clip();
        reserveAmmo = entity->reserveAmmoCount();
        id = entity->weaponId();

        if (const auto weaponData = entity->getWeaponInfo()) {
            type = weaponData->type;
            name = weaponData->name;
        }
    }
    int clip;
    int reserveAmmo;
    WeaponId id;
    WeaponType type = WeaponType::Unknown;
    std::string name;
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

    const auto observerTarget = localPlayer->getObserverMode() == ObsMode::InEye ? localPlayer->getObserverTarget() : nullptr;

    for (int i = 1; i <= memory->globalVars->maxClients; ++i) {
        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer || entity == observerTarget
            || entity->isDormant() || !entity->isAlive())
            continue;

        players.emplace_back(entity);
    }

    for (int i = memory->globalVars->maxClients + 1; i <= interfaces->entityList->getHighestEntityIndex(); ++i) {
        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity->isDormant())
            continue;

        if (entity->isWeapon()) {
            if (entity->ownerEntity() == -1)
                weapons.emplace_back(entity);
        } else {
            const auto classId = entity->getClientClass()->classId;

            switch (classId) {
            case ClassId::BaseCSGrenadeProjectile:
                if (entity->grenadeExploded())
                    break;
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
                entities.emplace_back(entity);
            }
        }
    }
}

struct BoundingBox {
private:
    bool valid;
public:
    ImVec2 min, max;
    ImVec2 vertices[8];

    BoundingBox(const BaseData& data) noexcept
    {
        const auto [width, height] = interfaces->engine->getScreenSize();

        min.x = static_cast<float>(width * 2);
        min.y = static_cast<float>(height * 2);
        max.x = -min.x;
        max.y = -min.y;

        const auto& mins = data.obbMins;
        const auto& maxs = data.obbMaxs;

        for (int i = 0; i < 8; ++i) {
            const Vector point{ i & 1 ? maxs.x : mins.x,
                                i & 2 ? maxs.y : mins.y,
                                i & 4 ? maxs.z : mins.z };

            if (!worldToScreen(point.transform(data.coordinateFrame), vertices[i])) {
                valid = false;
                return;
            }
            min.x = std::min(min.x, vertices[i].x);
            min.y = std::min(min.y, vertices[i].y);
            max.x = std::max(max.x, vertices[i].x);
            max.y = std::max(max.y, vertices[i].y);
        }
        valid = true;
    }

    operator bool() const noexcept
    {
        return valid;
    }
};

static void renderBox(ImDrawList* drawList, const BoundingBox& bbox, const Shared& config) noexcept
{
    if (!config.box.enabled)
        return;

    const ImU32 color = Helpers::calculateColor(config.box, memory->globalVars->realtime);

    switch (config.boxType) {
    case 0:
        drawList->AddRect(bbox.min, bbox.max, color, config.box.rounding, ImDrawCornerFlags_All, config.box.thickness);
        break;

    case 1:
        drawList->AddLine(bbox.min, { bbox.min.x, bbox.min.y * 0.75f + bbox.max.y * 0.25f }, color, config.box.thickness);
        drawList->AddLine(bbox.min, { bbox.min.x * 0.75f + bbox.max.x * 0.25f, bbox.min.y }, color, config.box.thickness);

        drawList->AddLine({ bbox.max.x, bbox.min.y }, { bbox.max.x * 0.75f + bbox.min.x * 0.25f, bbox.min.y }, color, config.box.thickness);
        drawList->AddLine({ bbox.max.x, bbox.min.y }, { bbox.max.x, bbox.min.y * 0.75f + bbox.max.y * 0.25f }, color, config.box.thickness);

        drawList->AddLine({ bbox.min.x, bbox.max.y }, { bbox.min.x, bbox.max.y * 0.75f + bbox.min.y * 0.25f }, color, config.box.thickness);
        drawList->AddLine({ bbox.min.x, bbox.max.y }, { bbox.min.x * 0.75f + bbox.max.x * 0.25f, bbox.max.y }, color, config.box.thickness);

        drawList->AddLine(bbox.max, { bbox.max.x * 0.75f + bbox.min.x * 0.25f, bbox.max.y }, color, config.box.thickness);
        drawList->AddLine(bbox.max, { bbox.max.x, bbox.max.y * 0.75f + bbox.min.y * 0.25f }, color, config.box.thickness);
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
                    drawList->AddLine(bbox.vertices[i], { bbox.vertices[i].x * 0.75f + bbox.vertices[i + j].x * 0.25f, bbox.vertices[i].y * 0.75f + bbox.vertices[i + j].y * 0.25f }, color, config.box.thickness);
                    drawList->AddLine({ bbox.vertices[i].x * 0.25f + bbox.vertices[i + j].x * 0.75f, bbox.vertices[i].y * 0.25f + bbox.vertices[i + j].y * 0.75f }, bbox.vertices[i + j], color, config.box.thickness);
                }
            }
        }
        break;
    }
}

static ImVec2 renderText(ImDrawList* drawList, float distance, float cullDistance, const Color& textCfg, const ColorToggleRounding& backgroundCfg, const char* text, const ImVec2& pos, bool centered = true, bool adjustHeight = true) noexcept
{
    if (cullDistance && Helpers::units2meters(distance) > cullDistance)
        return { };

    const auto fontSize = std::clamp(15.0f * 10.0f / std::sqrt(distance), 10.0f, 15.0f);

    const auto textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, -1.0f, text);
    const auto horizontalOffset = centered ? textSize.x / 2 : 0.0f;
    const auto verticalOffset = adjustHeight ? textSize.y : 0.0f;

    if (backgroundCfg.enabled) {
        const ImU32 color = Helpers::calculateColor(backgroundCfg, memory->globalVars->realtime);
        drawList->AddRectFilled({ pos.x - horizontalOffset - 2, pos.y - verticalOffset - 2 }, { pos.x - horizontalOffset + textSize.x + 2, pos.y - verticalOffset + textSize.y + 2 }, color, backgroundCfg.rounding);
    }
    const ImU32 color = Helpers::calculateColor(textCfg, memory->globalVars->realtime);
    drawList->AddText(nullptr, fontSize, { pos.x - horizontalOffset, pos.y - verticalOffset }, color, text);
    return textSize;
}

static void renderSnaplines(ImDrawList* drawList, const BoundingBox& bbox, const ColorToggleThickness& config, int type) noexcept
{
    if (!config.enabled)
        return;

    const auto [width, height] = interfaces->engine->getScreenSize();
    const ImU32 color = Helpers::calculateColor(config, memory->globalVars->realtime);
    drawList->AddLine({ static_cast<float>(width / 2), static_cast<float>(type == 0 ? height : type == 1 ? 0 : height / 2) }, { (bbox.min.x + bbox.max.x) / 2, type == 0 ? bbox.max.y : type == 1 ? bbox.min.y : (bbox.min.y + bbox.max.y) / 2 }, color, config.thickness);
}

static void renderPlayerBox(ImDrawList* drawList, const PlayerData& playerData, const Player& config) noexcept
{
    const BoundingBox bbox{ playerData };

    if (!bbox)
        return;

    renderBox(drawList, bbox, config);
    renderSnaplines(drawList, bbox, config.snaplines, config.snaplineType);

    ImGui::PushFont(::config->fonts[config.font.fullName]);

    ImVec2 flashDurationPos{ (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 12.5f };

    if (config.name.enabled && !playerData.name.empty()) {
        const auto nameSize = renderText(drawList, playerData.distanceToLocal, config.textCullDistance, config.name, config.textBackground, playerData.name.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
        flashDurationPos.y -= nameSize.y;
    }

    if (config.flashDuration.enabled && playerData.flashDuration > 0.0f) {
        drawList->PathArcTo(flashDurationPos, 5.0f, IM_PI / 2 - (playerData.flashDuration / 255.0f * IM_PI), IM_PI / 2 + (playerData.flashDuration / 255.0f * IM_PI));
        const ImU32 color = Helpers::calculateColor(config.flashDuration, memory->globalVars->realtime);
        drawList->PathStroke(color, false, 1.5f);
    }

    if (config.weapon.enabled && !playerData.activeWeapon.empty())
        renderText(drawList, playerData.distanceToLocal, config.textCullDistance, config.weapon, config.textBackground, playerData.activeWeapon.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);

    ImGui::PopFont();
}

static void renderWeaponBox(ImDrawList* drawList, const WeaponData& weaponData, const Weapon& config) noexcept
{
    const BoundingBox bbox{ weaponData };

    if (!bbox)
        return;

    renderBox(drawList, bbox, config);
    renderSnaplines(drawList, bbox, config.snaplines, config.snaplineType);

    ImGui::PushFont(::config->fonts[config.font.fullName]);

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

static void renderEntityBox(ImDrawList* drawList, const EntityData& entityData, const char* name, const Shared& config) noexcept
{
    const BoundingBox bbox{ entityData };

    if (!bbox)
        return;

    renderBox(drawList, bbox, config);
    renderSnaplines(drawList, bbox, config.snaplines, config.snaplineType);

    ImGui::PushFont(::config->fonts[config.font.fullName]);

    if (config.name.enabled)
        renderText(drawList, entityData.distanceToLocal, config.textCullDistance, config.name, config.textBackground, name, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });

    ImGui::PopFont();
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

static constexpr void renderWeaponEsp(ImDrawList* drawList, const WeaponData& weaponData, const Weapon& parentConfig, const Weapon& itemConfig) noexcept
{
    const auto& config = parentConfig.enabled ? parentConfig : itemConfig;
    if (config.enabled) {
        renderWeaponBox(drawList, weaponData, config);
    }
}

static void renderEntityEsp(ImDrawList* drawList, const EntityData& entityData, const Shared& parentConfig, const Shared& itemConfig, const char* name) noexcept
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
            constexpr auto dispatchWeapon = [](WeaponType type, int idx) -> std::optional<std::pair<const Weapon&, const Weapon&>> {
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
        constexpr auto dispatchEntity = [](ClassId classId, bool flashbang) -> std::optional<std::tuple<const Shared&, const Shared&, const char*>> {
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
