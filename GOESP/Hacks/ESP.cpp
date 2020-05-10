#define NOMINMAX
#include "ESP.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

#include "../Config.h"
#include "../fnv.h"
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
#include "../SDK/ModelInfo.h"
#include "../SDK/Sound.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponInfo.h"
#include "../SDK/WeaponId.h"

#include <limits>
#include <list>
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

static Vector localPlayerOrigin;

struct BaseData {
    BaseData(Entity* entity) noexcept
    {
        distanceToLocal = entity->getAbsOrigin().distTo(localPlayerOrigin);

        if (const auto model = entity->getModel()) {
            modelMins = model->mins;
            modelMaxs = model->maxs;
        }
            
        obbMins = entity->getCollideable()->obbMins();
        obbMaxs = entity->getCollideable()->obbMaxs();
        coordinateFrame = entity->toWorldTransform();
    }
    float distanceToLocal;
    Vector modelMins, modelMaxs;
    Vector obbMins, obbMaxs;
    Matrix3x4 coordinateFrame;
};

struct EntityData final : BaseData {
    EntityData(Entity* entity) noexcept : BaseData{ entity }
    {
        name = [](ClassId classId) -> const char* {
            switch (classId) {
            case ClassId::EconEntity: return "Defuse Kit";
            case ClassId::Chicken: return "Chicken";
            case ClassId::PlantedC4: return "Planted C4";
            case ClassId::Hostage: return "Hostage";
            case ClassId::Dronegun: return "Sentry";
            case ClassId::Cash: return "Cash";
            case ClassId::AmmoBox: return "Ammo Box";
            case ClassId::RadarJammer: return "Radar Jammer";
            case ClassId::SnowballPile: return "Snowball Pile";
            default: return nullptr;
            }
        }(entity->getClientClass()->classId);
    }
    const char* name;
};

struct ProjectileData : BaseData {
    ProjectileData(Entity* projectile) noexcept : BaseData{ projectile }
    {
        name = [](Entity* projectile) -> const char* {
            switch (projectile->getClientClass()->classId) {
            case ClassId::BaseCSGrenadeProjectile:
                if (const auto model = projectile->getModel(); model && std::strstr(model->name, "flashbang"))
                    return "Flashbang";
                else
                    return "HE Grenade";
            case ClassId::BreachChargeProjectile: return "Breach Charge";
            case ClassId::BumpMineProjectile: return "Bump Mine";
            case ClassId::DecoyProjectile: return "Decoy Grenade";
            case ClassId::MolotovProjectile: return "Molotov";
            case ClassId::SensorGrenadeProjectile: return "TA Grenade";
            case ClassId::SmokeGrenadeProjectile: return "Smoke Grenade";
            case ClassId::SnowballProjectile: return "Snowball";
            default: return nullptr;
            }
        }(projectile);

        if (const auto thrower = interfaces->entityList->getEntityFromHandle(projectile->thrower()); thrower && localPlayer) {
            if (thrower == localPlayer.get())
                thrownByLocalPlayer = true;
            else
                thrownByEnemy = memory->isOtherEnemy(localPlayer.get(), thrower);
        }

        handle = projectile->handle();
    }

    void update(Entity* projectile) noexcept
    {
        static_cast<BaseData&>(*this) = { projectile };

        if (const auto pos = projectile->getAbsOrigin(); trajectory.size() < 1 || trajectory[trajectory.size() - 1].second != pos)
            trajectory.emplace_back(memory->globalVars->realtime, pos);
    }

    constexpr auto operator==(int otherHandle) const noexcept
    {
        return handle == otherHandle;
    }

    bool exploded = false;
    bool thrownByLocalPlayer = false;
    bool thrownByEnemy = false;
    int handle;
    const char* name = nullptr;
    std::vector<std::pair<float, Vector>> trajectory;
};

struct PlayerData : BaseData {
    PlayerData(Entity* entity) noexcept : BaseData{ entity }
    {
        if (localPlayer) {
            enemy = memory->isOtherEnemy(entity, localPlayer.get());
            visible = entity->visibleTo(localPlayer.get());
        }

        constexpr auto isEntityAudible = [](int entityIndex) noexcept {
            for (int i = 0; i < memory->activeChannels->count; ++i)
                if (memory->channels[memory->activeChannels->list[i]].soundSource == entityIndex)
                    return true;
            return false;
        };

        audible = isEntityAudible(entity->index());
        spotted = entity->spotted();
        flashDuration = entity->flashDuration();
        name = entity->getPlayerName(config->normalizePlayerNames);

        if (const auto weapon = entity->getActiveWeapon()) {
            audible = audible || isEntityAudible(weapon->index());
            if (const auto weaponInfo = weapon->getWeaponInfo())
                activeWeapon = interfaces->localize->findAsUTF8(weaponInfo->name);
        }

        const auto model = entity->getModel();
        if (!model)
            return;

        const auto studioModel = interfaces->modelInfo->getStudioModel(model);
        if (!studioModel)
            return;

        Matrix3x4 boneMatrices[MAXSTUDIOBONES];
        if (!entity->setupBones(boneMatrices, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, memory->globalVars->currenttime))
            return;

        for (int i = 0; i < studioModel->numBones; ++i) {
            const auto bone = studioModel->getBone(i);

            if (!bone || bone->parent == -1 || !(bone->flags & BONE_USED_BY_HITBOX))
                continue;

            bones.emplace_back(boneMatrices[i].origin(), boneMatrices[bone->parent].origin());
        }
    }
    bool enemy = false;
    bool visible = false;
    bool audible;
    bool spotted;
    float flashDuration;
    std::string name;
    std::string activeWeapon;
    std::vector<std::pair<Vector, Vector>> bones;
};

struct WeaponData : BaseData {
    WeaponData(Entity* entity) noexcept : BaseData{ entity }
    {
        clip = entity->clip();
        reserveAmmo = entity->reserveAmmoCount();

        if (const auto weaponInfo = entity->getWeaponInfo()) {
            group = [](WeaponType type, WeaponId weaponId) {
                switch (type) {
                case WeaponType::Pistol: return "Pistols";
                case WeaponType::SubMachinegun: return "SMGs";
                case WeaponType::Rifle: return "Rifles";
                case WeaponType::SniperRifle: return "Sniper Rifles";
                case WeaponType::Shotgun: return "Shotguns";
                case WeaponType::Machinegun: return "Machineguns";
                case WeaponType::Grenade: return "Grenades";
                case WeaponType::Melee: return "Melee";
                default:
                    switch (weaponId) {
                    case WeaponId::C4:
                    case WeaponId::Healthshot:
                    case WeaponId::BumpMine:
                    case WeaponId::ZoneRepulsor:
                    case WeaponId::Shield:
                        return "Other";
                    default: return "All";
                    }
                }
            }(weaponInfo->type, entity->weaponId());
            name = [](WeaponId weaponId) {
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

                case WeaponId::Axe: return "Axe";
                case WeaponId::Hammer: return "Hammer";
                case WeaponId::Spanner: return "Wrench";

                case WeaponId::C4: return "C4";
                case WeaponId::Healthshot: return "Healthshot";
                case WeaponId::BumpMine: return "Bump Mine";
                case WeaponId::ZoneRepulsor: return "Zone Repulsor";
                case WeaponId::Shield: return "Shield";
                }
            }(entity->weaponId());

            displayName = interfaces->localize->findAsUTF8(weaponInfo->name);
        }
    }
    int clip;
    int reserveAmmo;
    const char* group = "All";
    const char* name = "All";
    std::string displayName;
};

struct LootCrateData : BaseData {
    LootCrateData(Entity* entity) noexcept : BaseData{ entity }
    {
        const auto model = entity->getModel();
        if (!model)
            return;

        name = [](const char* modelName) -> const char* {
            switch (fnv::hashRuntime(modelName)) {
            case fnv::hash("models/props_survival/cases/case_pistol.mdl"): return "Pistol Case";
            case fnv::hash("models/props_survival/cases/case_light_weapon.mdl"): return "Light Case";
            case fnv::hash("models/props_survival/cases/case_heavy_weapon.mdl"): return "Heavy Case";
            case fnv::hash("models/props_survival/cases/case_explosive.mdl"): return "Explosive Case";
            case fnv::hash("models/props_survival/cases/case_tools.mdl"): return "Tools Case";
            case fnv::hash("models/props_survival/cash/dufflebag.mdl"): return "Cash Dufflebag";
            default: return nullptr;
            }
        }(model->name);
    }
    const char* name = nullptr;
};

static std::vector<PlayerData> players;
static std::vector<WeaponData> weapons;
static std::vector<EntityData> entities;
static std::vector<LootCrateData> lootCrates;
static std::list<ProjectileData> projectiles;
static std::mutex dataMutex;

void ESP::collectData() noexcept
{
    std::scoped_lock _{ dataMutex };

    players.clear();
    weapons.clear();
    entities.clear();
    lootCrates.clear();

    if (!localPlayer)
        return;

    viewMatrix = interfaces->engine->worldToScreenMatrix();
    localPlayerOrigin = localPlayer->getAbsOrigin();

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
                if (entity->grenadeExploded()) {
                    if (const auto it = std::find(projectiles.begin(), projectiles.end(), entity->handle()); it != projectiles.end())
                        it->exploded = true;
                    break;
                }
            case ClassId::BreachChargeProjectile:
            case ClassId::BumpMineProjectile:
            case ClassId::DecoyProjectile:
            case ClassId::MolotovProjectile:
            case ClassId::SensorGrenadeProjectile:
            case ClassId::SmokeGrenadeProjectile:
            case ClassId::SnowballProjectile:
                if (const auto it = std::find(projectiles.begin(), projectiles.end(), entity->handle()); it != projectiles.end())
                    it->update(entity);
                else
                    projectiles.emplace_back(entity);
                break;
            case ClassId::EconEntity:
            case ClassId::Chicken:
            case ClassId::PlantedC4:
            case ClassId::Hostage:
            case ClassId::Dronegun:
            case ClassId::Cash:
            case ClassId::AmmoBox:
            case ClassId::RadarJammer:
            case ClassId::SnowballPile:
                entities.emplace_back(entity);
                break;
            case ClassId::LootCrate:
                lootCrates.emplace_back(entity);
            }
        }
    }

    for (auto it = projectiles.begin(); it != projectiles.end();) {
        if (!interfaces->entityList->getEntityFromHandle(it->handle)) {
            it->exploded = true;

            if (it->trajectory.size() < 1 || it->trajectory[it->trajectory.size() - 1].first + 60.0f < memory->globalVars->realtime) {
                it = projectiles.erase(it);
                continue;
            }
        }
        ++it;
    }
}

static constexpr auto operator-(float sub, const std::array<float, 3>& a) noexcept
{
    return Vector{ sub - a[0], sub - a[1], sub - a[2] };
}

struct BoundingBox {
private:
    bool valid;
public:
    ImVec2 min, max;
    ImVec2 vertices[8];

    BoundingBox(const BaseData& data, const std::array<float, 3>& scale, bool useModelBounds) noexcept
    {
        min.y = min.x = std::numeric_limits<float>::max();
        max.y = max.x = -std::numeric_limits<float>::max();

        const auto mins = useModelBounds ? data.modelMins + (data.modelMaxs - data.modelMins) * 2 * (0.25f - scale) : data.obbMins + (data.obbMaxs - data.obbMins) * 2 * (0.25f - scale);
        const auto maxs = useModelBounds ? data.modelMaxs - (data.modelMaxs - data.modelMins) * 2 * (0.25f - scale) : data.obbMaxs - (data.obbMaxs - data.obbMins) * 2 * (0.25f - scale);

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


    BoundingBox(const Vector& center) noexcept
    {
        min.y = min.x = std::numeric_limits<float>::max();
        max.y = max.x = -std::numeric_limits<float>::max();

        const auto mins = center - 2.0f;
        const auto maxs = center + 2.0f;

        for (int i = 0; i < 8; ++i) {
            const Vector point{ i & 1 ? maxs.x : mins.x,
                                i & 2 ? maxs.y : mins.y,
                                i & 4 ? maxs.z : mins.z };

            if (!worldToScreen(point, vertices[i])) {
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

static void renderBox(ImDrawList* drawList, const BoundingBox& bbox, const Box& config) noexcept
{
    if (!config.enabled)
        return;

    const ImU32 color = Helpers::calculateColor(config, memory->globalVars->realtime);

    switch (config.type) {
    case Box::_2d:
        drawList->AddRect(bbox.min, bbox.max, color, config.rounding, ImDrawCornerFlags_All, config.thickness);
        break;

    case Box::_2dCorners:
        drawList->AddLine(bbox.min, { bbox.min.x, bbox.min.y * 0.75f + bbox.max.y * 0.25f }, color, config.thickness);
        drawList->AddLine(bbox.min, { bbox.min.x * 0.75f + bbox.max.x * 0.25f, bbox.min.y }, color, config.thickness);

        drawList->AddLine({ bbox.max.x, bbox.min.y }, { bbox.max.x * 0.75f + bbox.min.x * 0.25f, bbox.min.y }, color, config.thickness);
        drawList->AddLine({ bbox.max.x, bbox.min.y }, { bbox.max.x, bbox.min.y * 0.75f + bbox.max.y * 0.25f }, color, config.thickness);

        drawList->AddLine({ bbox.min.x, bbox.max.y }, { bbox.min.x, bbox.max.y * 0.75f + bbox.min.y * 0.25f }, color, config.thickness);
        drawList->AddLine({ bbox.min.x, bbox.max.y }, { bbox.min.x * 0.75f + bbox.max.x * 0.25f, bbox.max.y }, color, config.thickness);

        drawList->AddLine(bbox.max, { bbox.max.x * 0.75f + bbox.min.x * 0.25f, bbox.max.y }, color, config.thickness);
        drawList->AddLine(bbox.max, { bbox.max.x, bbox.max.y * 0.75f + bbox.min.y * 0.25f }, color, config.thickness);
        break;
    case Box::_3d:
        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j))
                    drawList->AddLine(bbox.vertices[i], bbox.vertices[i + j], color, config.thickness);
            }
        }
        break;
    case Box::_3dCorners:
        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j)) {
                    drawList->AddLine(bbox.vertices[i], { bbox.vertices[i].x * 0.75f + bbox.vertices[i + j].x * 0.25f, bbox.vertices[i].y * 0.75f + bbox.vertices[i + j].y * 0.25f }, color, config.thickness);
                    drawList->AddLine({ bbox.vertices[i].x * 0.25f + bbox.vertices[i + j].x * 0.75f, bbox.vertices[i].y * 0.25f + bbox.vertices[i + j].y * 0.75f }, bbox.vertices[i + j], color, config.thickness);
                }
            }
        }
        break;
    }
}

static ImVec2 renderText(ImDrawList* drawList, const std::string& fontName, float distance, float cullDistance, const Color& textCfg, const ColorToggleRounding& backgroundCfg, const char* text, const ImVec2& pos, bool centered = true, bool adjustHeight = true) noexcept
{
    if (cullDistance && Helpers::units2meters(distance) > cullDistance)
        return { };

    constexpr auto fontSizeFromDist = [](float dist) constexpr noexcept {
        if (dist <= 200.0f)
            return 14;
        if (dist <= 500.0f)
            return 12;
        if (dist <= 1000.0f)
            return 10;
        return 8;
    };

    const int fontSize = fontSizeFromDist(distance);
    const auto font = config->fonts[fontName + ' ' + std::to_string(fontSize)];
    auto textSize = (font ? font : ImGui::GetFont())->CalcTextSizeA(static_cast<float>(fontSize), FLT_MAX, -1.0f, text);
    textSize.x = IM_FLOOR(textSize.x + 0.95f);

    const auto horizontalOffset = centered ? textSize.x / 2 : 0.0f;
    const auto verticalOffset = adjustHeight ? textSize.y : 0.0f;

    if (backgroundCfg.enabled) {
        const ImU32 color = Helpers::calculateColor(backgroundCfg, memory->globalVars->realtime);
        drawList->AddRectFilled({ pos.x - horizontalOffset - 2, pos.y - verticalOffset - 2 }, { pos.x - horizontalOffset + textSize.x + 2, pos.y - verticalOffset + textSize.y + 2 }, color, backgroundCfg.rounding);
    }
    const ImU32 color = Helpers::calculateColor(textCfg, memory->globalVars->realtime);
    drawList->AddText(font, static_cast<float>(fontSize), { pos.x - horizontalOffset, pos.y - verticalOffset }, color, text);
    return textSize;
}

static void drawSnapline(ImDrawList* drawList, const BoundingBox& bbox, const Snapline& config) noexcept
{
    if (!config.enabled)
        return;

    const auto [width, height] = interfaces->engine->getScreenSize();
    const ImU32 color = Helpers::calculateColor(config, memory->globalVars->realtime);
    drawList->AddLine({ static_cast<float>(width / 2), static_cast<float>(config.type == 0 ? height : config.type == 1 ? 0 : height / 2) }, { (bbox.min.x + bbox.max.x) / 2, config.type == 0 ? bbox.max.y : config.type == 1 ? bbox.min.y : (bbox.min.y + bbox.max.y) / 2 }, color, config.thickness);
}

static void renderPlayerBox(ImDrawList* drawList, const PlayerData& playerData, const Player& config) noexcept
{
    const BoundingBox bbox{ playerData, config.box.scale, config.useModelBounds };

    if (!bbox)
        return;

    renderBox(drawList, bbox, config.box);
    drawSnapline(drawList, bbox, config.snapline);

    ImVec2 flashDurationPos{ (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 12.5f };

    if (config.name.enabled && !playerData.name.empty()) {
        const auto nameSize = renderText(drawList, config.font.name, playerData.distanceToLocal, config.textCullDistance, config.name, config.textBackground, playerData.name.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
        flashDurationPos.y -= nameSize.y;
    }

    if (config.flashDuration.enabled && playerData.flashDuration > 0.0f) {
        drawList->PathArcTo(flashDurationPos, 5.0f, IM_PI / 2 - (playerData.flashDuration / 255.0f * IM_PI), IM_PI / 2 + (playerData.flashDuration / 255.0f * IM_PI));
        const ImU32 color = Helpers::calculateColor(config.flashDuration, memory->globalVars->realtime);
        drawList->PathStroke(color, false, 1.5f);
    }

    if (config.weapon.enabled && !playerData.activeWeapon.empty())
        renderText(drawList, config.font.name, playerData.distanceToLocal, config.textCullDistance, config.weapon, config.textBackground, playerData.activeWeapon.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);
}

static void renderWeaponBox(ImDrawList* drawList, const WeaponData& weaponData, const Weapon& config) noexcept
{
    const BoundingBox bbox{ weaponData, config.box.scale, config.useModelBounds };

    if (!bbox)
        return;

    renderBox(drawList, bbox, config.box);
    drawSnapline(drawList, bbox, config.snapline);

    if (config.name.enabled && !weaponData.displayName.empty()) {
        renderText(drawList, config.font.name, weaponData.distanceToLocal, config.textCullDistance, config.name, config.textBackground, weaponData.displayName.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
    }

    if (config.ammo.enabled && weaponData.clip != -1) {
        const auto text{ std::to_string(weaponData.clip) + " / " + std::to_string(weaponData.reserveAmmo) };
        renderText(drawList, config.font.name, weaponData.distanceToLocal, config.textCullDistance, config.ammo, config.textBackground, text.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);
    }
}

static void renderEntityBox(ImDrawList* drawList, const BaseData& entityData, const char* name, const Shared& config) noexcept
{
    const BoundingBox bbox{ entityData, config.box.scale, config.useModelBounds };

    if (!bbox)
        return;

    renderBox(drawList, bbox, config.box);
    drawSnapline(drawList, bbox, config.snapline);

    if (config.name.enabled)
        renderText(drawList, config.font.name, entityData.distanceToLocal, config.textCullDistance, config.name, config.textBackground, name, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
}

static void drawProjectileTrajectory(ImDrawList* drawList, const Trail& config, const std::vector<std::pair<float, Vector>>& trajectory) noexcept
{
    if (!config.enabled)
        return;

    std::vector<ImVec2> points;

    const auto color = Helpers::calculateColor(config, memory->globalVars->realtime);

    for (const auto& [time, point] : trajectory) {
        if (ImVec2 pos; time + config.time >= memory->globalVars->realtime && worldToScreen(point, pos)) {
            if (config.type == Trail::Line)
                points.push_back(pos);
            else if (config.type == Trail::Circles)
                drawList->AddCircle(pos, 3.5f - point.distTo(localPlayerOrigin) / 700.0f, color, 12, config.thickness);
            else if (config.type == Trail::FilledCircles)
                drawList->AddCircleFilled(pos, 3.5f - point.distTo(localPlayerOrigin) / 700.0f, color);
        }
    }

    if (config.type == Trail::Line)
        drawList->AddPolyline(points.data(), points.size(), color, false, config.thickness);
}

static void drawPlayerSkeleton(ImDrawList* drawList, const ColorToggleThickness& config, const std::vector<std::pair<Vector, Vector>>& bones) noexcept
{
    if (!config.enabled)
        return;

    for (const auto& [bone, parent] : bones) {
        ImVec2 bonePoint;
        if (!worldToScreen(bone, bonePoint))
            continue;

        ImVec2 parentPoint;
        if (!worldToScreen(parent, parentPoint))
            continue;

        const auto color = Helpers::calculateColor(config, memory->globalVars->realtime);
        drawList->AddLine(bonePoint, parentPoint, color, config.thickness);
    }
}

static bool renderPlayerEsp(ImDrawList* drawList, const PlayerData& playerData, const Player& playerConfig) noexcept
{
    if (!playerConfig.enabled)
        return false;

    if (playerConfig.audibleOnly && !playerData.audible && !playerConfig.spottedOnly
     || playerConfig.spottedOnly && !playerData.spotted && !(playerConfig.audibleOnly && playerData.audible)) // if both "Audible Only" and "Spotted Only" are on treat them as audible OR spotted
        return true;

    renderPlayerBox(drawList, playerData, playerConfig);
    drawPlayerSkeleton(drawList, playerConfig.skeleton, playerData.bones);

    return true;
}

static void renderWeaponEsp(ImDrawList* drawList, const WeaponData& weaponData, const Weapon& parentConfig, const Weapon& itemConfig) noexcept
{
    const auto& config = itemConfig.enabled ? itemConfig : (parentConfig.enabled ? parentConfig : ::config->weapons["All"]);
    if (config.enabled) {
        renderWeaponBox(drawList, weaponData, config);
    }
}

static void renderEntityEsp(ImDrawList* drawList, const BaseData& entityData, const Shared& parentConfig, const Shared& itemConfig, const char* name) noexcept
{
    const auto& config = itemConfig.enabled ? itemConfig : parentConfig;

    if (config.enabled) {
        renderEntityBox(drawList, entityData, name, config);
    }
}

static void renderProjectileEsp(ImDrawList* drawList, const ProjectileData& projectileData, const Projectile& parentConfig, const Projectile& itemConfig, const char* name) noexcept
{
    const auto& config = itemConfig.enabled ? itemConfig : parentConfig;

    if (config.enabled) {
        if (!projectileData.exploded)
            renderEntityBox(drawList, projectileData, name, config);

        if (config.trails.enabled) {
            if (projectileData.thrownByLocalPlayer)
                drawProjectileTrajectory(drawList, config.trails.localPlayer, projectileData.trajectory);
            else if (!projectileData.thrownByEnemy)
                drawProjectileTrajectory(drawList, config.trails.allies, projectileData.trajectory);
            else
                drawProjectileTrajectory(drawList, config.trails.enemies, projectileData.trajectory);
        }
    }
}

void ESP::render(ImDrawList* drawList) noexcept
{
    std::scoped_lock _{ dataMutex };

    for (const auto& player : players) {
        auto& playerConfig = player.enemy ? config->enemies : config->allies;

        if (!renderPlayerEsp(drawList, player, playerConfig["All"]))
            renderPlayerEsp(drawList, player, playerConfig[player.visible ? "Visible" : "Occluded"]);
    }

    for (const auto& weapon : weapons)
        renderWeaponEsp(drawList, weapon, config->weapons[weapon.group], config->weapons[weapon.name]);

    // TODO: reduce code duplication
    for (const auto& entity : entities) {
        if (entity.name)
            renderEntityEsp(drawList, entity, config->otherEntities["All"], config->otherEntities[entity.name], entity.name);
    }

    for (const auto& lootCrate : lootCrates) {
        if (lootCrate.name)
            renderEntityEsp(drawList, lootCrate, config->lootCrates["All"], config->lootCrates[lootCrate.name], lootCrate.name);
    }

    for (const auto& projectile : projectiles) {
        if (projectile.name)
            renderProjectileEsp(drawList, projectile, config->projectiles["All"], config->projectiles[projectile.name], projectile.name);
    }
}

void ESP::clearProjectileList() noexcept
{
    std::scoped_lock _{ dataMutex };
    
    projectiles.clear();
}
