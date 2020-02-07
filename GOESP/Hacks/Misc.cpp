#include "Misc.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

#include "../Config.h"
#include "../Helpers.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../SDK/ConVar.h"
#include "../SDK/Cvar.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/GlobalVars.h"

#include <mutex>

struct LocalPlayerData {
    bool exists;
    bool alive;
    bool inReload;
    float nextWeaponAttack;
    Vector aimPunch;
};

static LocalPlayerData localPlayer;
static std::mutex dataMutex;

void Misc::collectData() noexcept
{
    std::scoped_lock _{ dataMutex };

    const auto local = interfaces->entityList->getEntity(interfaces->engine->getLocalPlayer());
    localPlayer.exists = local;

    if (local) {
        localPlayer.alive = local->isAlive();

        if (const auto activeWeapon = local->getActiveWeapon()) {
            localPlayer.inReload = activeWeapon->isInReload();
            localPlayer.nextWeaponAttack = activeWeapon->nextPrimaryAttack();
        } else {
            localPlayer.inReload = false;
            localPlayer.nextWeaponAttack = 0.0f;
        }
        localPlayer.aimPunch = local->getAimPunch();
    }
}

void Misc::drawReloadProgress(ImDrawList* drawList) noexcept
{
    if (config->reloadProgress.enabled && interfaces->engine->isInGame()) {
        std::scoped_lock _{ dataMutex };

        if (!(localPlayer.exists && localPlayer.alive))
            return;

        static float reloadLength = 0.0f;

        if (localPlayer.inReload) {
            if (!reloadLength)
                reloadLength = localPlayer.nextWeaponAttack - memory->globalVars->currenttime;

            const auto [width, height] = interfaces->engine->getScreenSize();
            constexpr int segments = 20;
            drawList->PathArcTo({ width / 2.0f, height / 2.0f }, 20.0f, -IM_PI / 2, std::clamp(IM_PI * 2 * (0.75f - (localPlayer.nextWeaponAttack - memory->globalVars->currenttime) / reloadLength), -IM_PI / 2, -IM_PI / 2 + IM_PI * 2), segments);
            const ImU32 color = Helpers::calculateColor(config->reloadProgress.color, config->reloadProgress.rainbow, config->reloadProgress.rainbowSpeed, memory->globalVars->realtime);
            drawList->PathStroke(color, false, config->reloadProgress.thickness);
        } else {
            reloadLength = 0.0f;
        }
    }
}

void Misc::drawRecoilCrosshair(ImDrawList* drawList) noexcept
{
    if (!config->recoilCrosshair.enabled)
        return;

    std::scoped_lock _{ dataMutex };

    if (!localPlayer.exists || !localPlayer.alive)
        return;

    const auto [width, height] = interfaces->engine->getScreenSize();

    const float x = width * (0.5f - localPlayer.aimPunch.y / 180.0f);
    const float y = height * (0.5f + localPlayer.aimPunch.x / 180.0f);
    const auto color = Helpers::calculateColor(config->recoilCrosshair.color, config->recoilCrosshair.rainbow, config->recoilCrosshair.rainbowSpeed, memory->globalVars->realtime);

    drawList->AddLine({ x, y - 10 }, { x, y + 10 }, color, config->recoilCrosshair.thickness);
    drawList->AddLine({ x - 10, y }, { x + 10, y }, color, config->recoilCrosshair.thickness);
}
