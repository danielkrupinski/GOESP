#define NOMINMAX
#include "ESP.h"

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include "../Config.h"
#include "../fnv.h"
#include "../GameData.h"
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
#include <tuple>

static bool worldToScreen(const Vector& in, ImVec2& out) noexcept
{
    const auto& matrix = GameData::toScreenMatrix();

    const auto w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;
    if (w < 0.001f)
        return false;

    out = ImGui::GetIO().DisplaySize / 2.0f;
    out.x *= 1.0f + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w;
    out.y *= 1.0f - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w;
    return true;
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

    BoundingBox(const BaseData& data, const std::array<float, 3>& scale) noexcept
    {
        min.y = min.x = std::numeric_limits<float>::max();
        max.y = max.x = -std::numeric_limits<float>::max();

        const auto& mins = data.obbMins;
        const auto& maxs = data.obbMaxs;

        const auto scaledMins = mins + (maxs - mins) * 2 * (0.25f - scale);
        const auto scaledMaxs = maxs - (maxs - mins) * 2 * (0.25f - scale);

        for (int i = 0; i < 8; ++i) {
            const Vector point{ i & 1 ? scaledMaxs.x : scaledMins.x,
                                i & 2 ? scaledMaxs.y : scaledMins.y,
                                i & 4 ? scaledMaxs.z : scaledMins.z };

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

static ImDrawList* drawList;

static void addLineWithShadow(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness) noexcept
{
    drawList->AddLine(p1 + ImVec2{ 1.0f, 1.0f }, p2 + ImVec2{ 1.0f, 1.0f }, col & IM_COL32_A_MASK, thickness);
    drawList->AddLine(p1, p2, col, thickness);
}

static void renderBox(const BoundingBox& bbox, const Box& config) noexcept
{
    if (!config.enabled)
        return;

    const ImU32 color = Helpers::calculateColor(config);

    switch (config.type) {
    case Box::_2d:
        drawList->AddRect(bbox.min + ImVec2{ 1.0f, 1.0f }, bbox.max + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK, config.rounding, ImDrawCornerFlags_All, config.thickness);
        drawList->AddRect(bbox.min, bbox.max, color, config.rounding, ImDrawCornerFlags_All, config.thickness);
        break;
    case Box::_2dCorners:
        addLineWithShadow(bbox.min, { bbox.min.x, bbox.min.y * 0.75f + bbox.max.y * 0.25f }, color, config.thickness);
        addLineWithShadow(bbox.min, { bbox.min.x * 0.75f + bbox.max.x * 0.25f, bbox.min.y }, color, config.thickness);

        addLineWithShadow({ bbox.max.x, bbox.min.y }, { bbox.max.x * 0.75f + bbox.min.x * 0.25f, bbox.min.y }, color, config.thickness);
        addLineWithShadow({ bbox.max.x, bbox.min.y }, { bbox.max.x, bbox.min.y * 0.75f + bbox.max.y * 0.25f }, color, config.thickness);

        addLineWithShadow({ bbox.min.x, bbox.max.y }, { bbox.min.x, bbox.max.y * 0.75f + bbox.min.y * 0.25f }, color, config.thickness);
        addLineWithShadow({ bbox.min.x, bbox.max.y }, { bbox.min.x * 0.75f + bbox.max.x * 0.25f, bbox.max.y }, color, config.thickness);

        addLineWithShadow(bbox.max, { bbox.max.x * 0.75f + bbox.min.x * 0.25f, bbox.max.y }, color, config.thickness);
        addLineWithShadow(bbox.max, { bbox.max.x, bbox.max.y * 0.75f + bbox.min.y * 0.25f }, color, config.thickness);
        break;
    case Box::_3d:
        // two separate loops to make shadows not overlap normal lines
        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j))
                    drawList->AddLine(bbox.vertices[i] + ImVec2{ 1.0f, 1.0f }, bbox.vertices[i + j] + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK, config.thickness);
            }
        }
        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j))
                    drawList->AddLine(bbox.vertices[i], bbox.vertices[i + j], color, config.thickness);
            }
        }
        break;
    case Box::_3dCorners:
        // two separate loops to make shadows not overlap normal lines
        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j)) {
                    drawList->AddLine(bbox.vertices[i] + ImVec2{ 1.0f, 1.0f }, ImVec2{ bbox.vertices[i].x * 0.75f + bbox.vertices[i + j].x * 0.25f, bbox.vertices[i].y * 0.75f + bbox.vertices[i + j].y * 0.25f } + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK, config.thickness);
                    drawList->AddLine(ImVec2{ bbox.vertices[i].x * 0.25f + bbox.vertices[i + j].x * 0.75f, bbox.vertices[i].y * 0.25f + bbox.vertices[i + j].y * 0.75f } + ImVec2{ 1.0f, 1.0f }, bbox.vertices[i + j] + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK, config.thickness);
                }
            }
        }

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

static ImVec2 renderText(float distance, float cullDistance, const Color& textCfg, const char* text, const ImVec2& pos, bool centered = true, bool adjustHeight = true) noexcept
{
    if (cullDistance && Helpers::units2meters(distance) > cullDistance)
        return { };

    const auto textSize = ImGui::CalcTextSize(text);

    const auto horizontalOffset = centered ? textSize.x / 2 : 0.0f;
    const auto verticalOffset = adjustHeight ? textSize.y : 0.0f;

    const auto color = Helpers::calculateColor(textCfg);
    drawList->AddText({ pos.x - horizontalOffset + 1.0f, pos.y - verticalOffset + 1.0f }, color & IM_COL32_A_MASK, text);
    drawList->AddText({ pos.x - horizontalOffset, pos.y - verticalOffset }, color, text);

    return textSize;
}

static void drawSnapline(const BoundingBox& bbox, const Snapline& config) noexcept
{
    if (!config.enabled)
        return;
    
    const auto& screenSize = ImGui::GetIO().DisplaySize;
    
    ImVec2 p1, p2;
    p1.x = screenSize.x / 2;
    p2.x = (bbox.min.x + bbox.max.x) / 2;

    switch (config.type) {
    case Snapline::Bottom:
        p1.y = screenSize.y;
        p2.y = bbox.max.y;
        break;
    case Snapline::Top:
        p1.y = 0.0f;
        p2.y = bbox.min.y;
        break;
    case Snapline::Crosshair:
        p1.y = screenSize.y / 2;
        p2.y = (bbox.min.y + bbox.max.y) / 2;
        break;
    default:
        return;
    }

    drawList->AddLine(p1, p2, Helpers::calculateColor(config), config.thickness);
}

struct FontPush {
    FontPush(const std::string& name, float distance)
    {
        constexpr auto fontSizeFromDist = [](float dist) constexpr noexcept {
            if (dist <= 200.0f)
                return 14;
            if (dist <= 500.0f)
                return 12;
            if (dist <= 1000.0f)
                return 10;
            return 8;
        };

        ImGui::PushFont(config->fonts[name + ' ' + std::to_string(fontSizeFromDist(distance))]);
    }

    ~FontPush()
    {
        ImGui::PopFont();
    }
};

static void renderPlayerBox(const PlayerData& playerData, const Player& config) noexcept
{
    const BoundingBox bbox{ playerData, config.box.scale };

    if (!bbox)
        return;
    
    renderBox(bbox, config.box);
    drawSnapline(bbox, config.snapline);

    ImVec2 flashDurationPos{ (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 7.5f };

    FontPush font{ config.font.name, playerData.distanceToLocal };

    if (config.name.enabled) {
        const auto nameSize = renderText(playerData.distanceToLocal, config.textCullDistance, config.name, playerData.name, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
        flashDurationPos.y -= nameSize.y;
    }

    if (config.flashDuration.enabled && playerData.flashDuration > 0.0f) {
        const auto radius = std::max(5.0f - playerData.distanceToLocal / 600.0f, 1.0f);
        flashDurationPos.y -= radius;

        const auto color = Helpers::calculateColor(config.flashDuration);
        drawList->PathArcTo(flashDurationPos + ImVec2{ 1.0f, 1.0f }, radius, IM_PI / 2 - (playerData.flashDuration / 255.0f * IM_PI), IM_PI / 2 + (playerData.flashDuration / 255.0f * IM_PI), 40);
        drawList->PathStroke(color & IM_COL32_A_MASK, false, 0.9f + radius * 0.1f);

        drawList->PathArcTo(flashDurationPos, radius, IM_PI / 2 - (playerData.flashDuration / 255.0f * IM_PI), IM_PI / 2 + (playerData.flashDuration / 255.0f * IM_PI), 40);
        drawList->PathStroke(color, false, 0.9f + radius * 0.1f);
    }

    if (config.weapon.enabled && !playerData.activeWeapon.empty())
        renderText(playerData.distanceToLocal, config.textCullDistance, config.weapon, playerData.activeWeapon.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);
}

static void renderWeaponBox(const WeaponData& weaponData, const Weapon& config) noexcept
{
    const BoundingBox bbox{ weaponData, config.box.scale };

    if (!bbox)
        return;

    renderBox(bbox, config.box);
    drawSnapline(bbox, config.snapline);

    FontPush font{ config.font.name, weaponData.distanceToLocal };

    if (config.name.enabled && !weaponData.displayName.empty()) {
        renderText(weaponData.distanceToLocal, config.textCullDistance, config.name, weaponData.displayName.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
    }

    if (config.ammo.enabled && weaponData.clip != -1) {
        const auto text{ std::to_string(weaponData.clip) + " / " + std::to_string(weaponData.reserveAmmo) };
        renderText(weaponData.distanceToLocal, config.textCullDistance, config.ammo, text.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);
    }
}

static void renderEntityBox(const BaseData& entityData, const char* name, const Shared& config) noexcept
{
    const BoundingBox bbox{ entityData, config.box.scale };

    if (!bbox)
        return;

    renderBox(bbox, config.box);
    drawSnapline(bbox, config.snapline);

    FontPush font{ config.font.name, entityData.distanceToLocal };

    if (config.name.enabled)
        renderText(entityData.distanceToLocal, config.textCullDistance, config.name, name, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
}

static void drawProjectileTrajectory(const Trail& config, const std::vector<std::pair<float, Vector>>& trajectory) noexcept
{
    if (!config.enabled)
        return;

    std::vector<ImVec2> points;

    const auto color = Helpers::calculateColor(config);

    for (const auto& [time, point] : trajectory) {
        if (ImVec2 pos; time + config.time >= memory->globalVars->realtime && worldToScreen(point, pos)) {
            if (config.type == Trail::Line)
                points.push_back(pos);
            else if (config.type == Trail::Circles)
                drawList->AddCircle(pos, 3.5f - point.distTo(GameData::local().origin) / 700.0f, color, 12, config.thickness);
            else if (config.type == Trail::FilledCircles)
                drawList->AddCircleFilled(pos, 3.5f - point.distTo(GameData::local().origin) / 700.0f, color);
        }
    }

    if (config.type == Trail::Line)
        drawList->AddPolyline(points.data(), points.size(), color, false, config.thickness);
}

static void drawPlayerSkeleton(const ColorToggleThickness& config, const std::vector<std::pair<Vector, Vector>>& bones) noexcept
{
    if (!config.enabled)
        return;

    const auto color = Helpers::calculateColor(config);

    for (const auto& [bone, parent] : bones) {
        ImVec2 bonePoint;
        if (!worldToScreen(bone, bonePoint))
            continue;

        ImVec2 parentPoint;
        if (!worldToScreen(parent, parentPoint))
            continue;

        drawList->AddLine(bonePoint, parentPoint, color, config.thickness);
    }
}

static bool renderPlayerEsp(const PlayerData& playerData, const Player& playerConfig) noexcept
{
    if (!playerConfig.enabled)
        return false;

    if (playerConfig.audibleOnly && !playerData.audible && !playerConfig.spottedOnly
     || playerConfig.spottedOnly && !playerData.spotted && !(playerConfig.audibleOnly && playerData.audible)) // if both "Audible Only" and "Spotted Only" are on treat them as audible OR spotted
        return true;

    renderPlayerBox(playerData, playerConfig);
    drawPlayerSkeleton(playerConfig.skeleton, playerData.bones);

    return true;
}

static void renderWeaponEsp(const WeaponData& weaponData, const Weapon& parentConfig, const Weapon& itemConfig) noexcept
{
    const auto& config = itemConfig.enabled ? itemConfig : (parentConfig.enabled ? parentConfig : ::config->weapons["All"]);
    if (config.enabled) {
        renderWeaponBox(weaponData, config);
    }
}

static void renderEntityEsp(const BaseData& entityData, const std::unordered_map<std::string, Shared>& map, const char* name) noexcept
{
    if (const auto cfg = map.find(name); cfg != map.cend() && cfg->second.enabled) {
        renderEntityBox(entityData, name, cfg->second);
    } else if (const auto cfg = map.find("All"); cfg != map.cend() && cfg->second.enabled) {
        renderEntityBox(entityData, name, cfg->second);
    }
}

static void renderProjectileEsp(const ProjectileData& projectileData, const Projectile& parentConfig, const Projectile& itemConfig, const char* name) noexcept
{
    const auto& config = itemConfig.enabled ? itemConfig : parentConfig;

    if (config.enabled) {
        if (!projectileData.exploded)
            renderEntityBox(projectileData, name, config);

        if (config.trails.enabled) {
            if (projectileData.thrownByLocalPlayer)
                drawProjectileTrajectory(config.trails.localPlayer, projectileData.trajectory);
            else if (!projectileData.thrownByEnemy)
                drawProjectileTrajectory(config.trails.allies, projectileData.trajectory);
            else
                drawProjectileTrajectory(config.trails.enemies, projectileData.trajectory);
        }
    }
}

void ESP::render() noexcept
{
    drawList = ImGui::GetBackgroundDrawList();

    GameData::Lock lock;

    for (const auto& weapon : GameData::weapons())
        renderWeaponEsp(weapon, config->weapons[weapon.group], config->weapons[weapon.name]);

    for (const auto& entity : GameData::entities())
        renderEntityEsp(entity, config->otherEntities, entity.name);

    for (const auto& lootCrate : GameData::lootCrates()) {
        if (lootCrate.name)
            renderEntityEsp(lootCrate, config->lootCrates, lootCrate.name);
    }

    for (const auto& projectile : GameData::projectiles())
        renderProjectileEsp(projectile, config->projectiles["All"], config->projectiles[projectile.name], projectile.name);

    for (const auto& player : GameData::players()) {
        auto& playerConfig = player.enemy ? config->enemies : config->allies;

        if (!renderPlayerEsp(player, playerConfig["All"]))
            renderPlayerEsp(player, playerConfig[player.visible ? "Visible" : "Occluded"]);
    }
}
