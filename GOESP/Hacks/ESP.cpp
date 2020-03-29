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
#include "../SDK/LocalPlayer.h"
#include "../SDK/Sound.h"
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
        if (!localPlayer)
            return;
        
        constexpr auto isEntityAudible = [](int entityIndex) noexcept {
            for (int i = 0; i < memory->activeChannels->count; ++i)
                if (memory->channels[memory->activeChannels->list[i]].soundSource == entityIndex)
                    return true;

            return false;
        };

        enemy = memory->isOtherEnemy(entity, localPlayer.get());
        visible = entity->visibleTo(localPlayer.get());
        audible = isEntityAudible(entity->index());

        flashDuration = entity->flashDuration();

        name = entity->getPlayerName(config->normalizePlayerNames);

        if (const auto weapon = entity->getActiveWeapon()) {
            audible = audible || isEntityAudible(weapon->index());
            if (const auto weaponData = weapon->getWeaponInfo()) {
                if (char weaponName[100]; WideCharToMultiByte(CP_UTF8, 0, interfaces->localize->find(weaponData->name), -1, weaponName, _countof(weaponName), nullptr, nullptr))
                    activeWeapon = weaponName;
            }
        }
    }
    bool enemy;
    bool visible;
    bool audible;
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

    if (!localPlayer)
        return;

    viewMatrix = interfaces->engine->worldToScreenMatrix();

    const auto observerTarget = localPlayer->getObserverMode() == ObsMode::InEye ? localPlayer->getObserverTarget() : nullptr;

    for (int i = 1; i <= memory->globalVars->maxClients; ++i) {
        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer.get() || entity == observerTarget
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
            switch (entity->getClientClass()->classId) {
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

static constexpr bool renderPlayerEsp(ImDrawList* drawList, const PlayerData& playerData, const Player& playerConfig) noexcept
{
    if (playerConfig.enabled && (!playerConfig.audibleOnly || playerData.audible)) {
        renderPlayerBox(drawList, playerData, playerConfig);
    }
    return playerConfig.enabled;
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
            if (!renderPlayerEsp(drawList, player, config->allies["All"])) {
                if (player.visible)
                    renderPlayerEsp(drawList, player, config->allies["Visible"]);
                else
                    renderPlayerEsp(drawList, player, config->allies["Occluded"]);
            }
        } else if (!renderPlayerEsp(drawList, player, config->enemies["All"])) {
            if (player.visible)
                renderPlayerEsp(drawList, player, config->enemies["Visible"]);
            else
                renderPlayerEsp(drawList, player, config->enemies["Occluded"]);
        }
    }

    for (const auto& weapon : weapons) {
        constexpr auto getWeaponIndex = [](WeaponId weaponId) constexpr noexcept {
            switch (weaponId) {
            default: return "All";

            case WeaponId::Glock: return "Glock-18";
            case WeaponId::Hkp2000: return "P2000";
            case WeaponId::Usp_s: return "USP-S";
            case WeaponId::Elite: return "Dual Berettas";
            case WeaponId::P250: return "P250";
            case WeaponId::Tec9: return "Tec-9";
            case WeaponId::Fiveseven: return "Five-SeveN";
            case WeaponId::Cz75a: return "CZ75-Auto";
            case WeaponId::Deagle: return "Desert Eagle";
            case WeaponId::Revolver: return "R8 Revolver";
            
            case WeaponId::Mac10: return "MAC-10";
            case WeaponId::Mp9: return "MP9";
            case WeaponId::Mp7: return "MP7";
            case WeaponId::Mp5sd: return "MP5-SD";
            case WeaponId::Ump45: return "UMP-45";
            case WeaponId::P90: return "P90";
            case WeaponId::Bizon: return "PP-Bizon";

            case WeaponId::GalilAr: return "Galil AR";
            case WeaponId::Famas: return "FAMAS";
            case WeaponId::Ak47: return "AK-47";
            case WeaponId::M4A1: return "M4A4";
            case WeaponId::M4a1_s: return "M4A1-S";
            case WeaponId::Sg553: return "SG 553";
            case WeaponId::Aug: return "AUG";

            case WeaponId::Ssg08: return "SSG 08";
            case WeaponId::Awp: return "AWP";
            case WeaponId::G3SG1: return "G3SG1";
            case WeaponId::Scar20: return "SCAR-20";

            case WeaponId::Nova: return "Nova";
            case WeaponId::Xm1014: return "XM1014";
            case WeaponId::Sawedoff: return "Sawed-Off";
            case WeaponId::Mag7: return "MAG-7";
            
            case WeaponId::M249: return "M249";
            case WeaponId::Negev: return "Negev";

            case WeaponId::Flashbang: return "Flashbang";
            case WeaponId::HeGrenade: return "HE Grenade";
            case WeaponId::SmokeGrenade: return "Smoke Grenade";
            case WeaponId::Molotov: return "Molotov";
            case WeaponId::Decoy: return "Decoy Grenade";
            case WeaponId::IncGrenade: return "Incendiary";
            case WeaponId::TaGrenade: return "TA Grenade";
            case WeaponId::Firebomb: return "Fire Bomb";
            case WeaponId::Diversion: return "Diversion";
            case WeaponId::FragGrenade: return "Frag Grenade";
            case WeaponId::Snowball: return "Snowball";
            }
        };

        if (!config->_weapons["All"].enabled) {
            constexpr auto getWeaponType = [](WeaponType type) constexpr noexcept {
                switch (type) {
                case WeaponType::Pistol: return "Pistols";
                case WeaponType::SubMachinegun: return "SMGs";
                case WeaponType::Rifle: return "Rifles";
                case WeaponType::SniperRifle: return "Sniper Rifles";
                case WeaponType::Shotgun: return "Shotguns";
                case WeaponType::Machinegun: return "Machineguns";
                case WeaponType::Grenade: return "Grenades";
                default: return "All";
                }
            };

            renderWeaponEsp(drawList, weapon, config->_weapons[getWeaponType(weapon.type)], config->_weapons[getWeaponIndex(weapon.id)]);
        } else {
            renderWeaponEsp(drawList, weapon, config->_weapons["All"], config->_weapons["All"]);
        }
    }

    for (const auto& entity : entities) {
        constexpr auto dispatchEntity = [](ClassId classId, bool flashbang) -> std::optional<std::tuple<const Shared&, const Shared&, const char*>> {
            switch (classId) {
            case ClassId::BaseCSGrenadeProjectile:
                if (flashbang) return  { { config->_projectiles["All"],  config->_projectiles["Flashbang"], "Flashbang" } };
                else return  { { config->_projectiles["All"], config->_projectiles["HE Grenade"], "HE Grenade" } };
            case ClassId::BreachChargeProjectile: return { { config->_projectiles["All"], config->_projectiles["Breach Charge"], "Breach Charge" } };
            case ClassId::BumpMineProjectile: return { { config->_projectiles["All"], config->_projectiles["Bump Mine"], "Bump Mine" } };
            case ClassId::DecoyProjectile: return  { { config->_projectiles["All"], config->_projectiles["Decoy Grenade"], "Decoy Grenade" } };
            case ClassId::MolotovProjectile: return { { config->_projectiles["All"], config->_projectiles["Molotov"], "Molotov" } };
            case ClassId::SensorGrenadeProjectile: return { { config->_projectiles["All"], config->_projectiles["TA Grenade"], "TA Grenade" } };
            case ClassId::SmokeGrenadeProjectile: return { { config->_projectiles["All"], config->_projectiles["Smoke Grenade"], "Smoke Grenade" } };
            case ClassId::SnowballProjectile: return { { config->_projectiles["All"], config->_projectiles["Snowball"], "Snowball" } };

            case ClassId::EconEntity: return { { config->_otherEntities["All"], config->_otherEntities["Defuse Kit"], "Defuse Kit" } };
            case ClassId::Chicken: return { { config->_otherEntities["All"], config->_otherEntities["Chicken"], "Chicken" } };
            case ClassId::PlantedC4: return { { config->_otherEntities["All"], config->_otherEntities["Planted C4"], "Planted C4" } };
            default: return std::nullopt;
            }
        };

        if (const auto e = dispatchEntity(entity.classId, entity.flashbang))
            renderEntityEsp(drawList, entity, std::get<0>(*e), std::get<1>(*e), std::get<2>(*e));
    }
}
