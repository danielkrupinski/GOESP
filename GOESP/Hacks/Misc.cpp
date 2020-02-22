#include "Misc.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

#include "../Config.h"
#include "../fnv.h"
#include "../GUI.h"
#include "../Helpers.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../SDK/ConVar.h"
#include "../SDK/Cvar.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/GameEvent.h"
#include "../SDK/GlobalVars.h"

#include <cassert>
#include <mutex>
#include <numeric>
#include <unordered_map>
#include <vector>

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
    if (!config->reloadProgress.enabled)
        return;

    std::scoped_lock _{ dataMutex };

    if (!localPlayer.exists || !localPlayer.alive)
        return;

    static float reloadLength = 0.0f;

    if (localPlayer.inReload) {
        if (!reloadLength)
            reloadLength = localPlayer.nextWeaponAttack - memory->globalVars->currenttime;

        const auto [width, height] = interfaces->engine->getScreenSize();
        constexpr int segments = 20;
        drawList->PathArcTo({ width / 2.0f, height / 2.0f }, 20.0f, -IM_PI / 2, std::clamp(IM_PI * 2 * (0.75f - (localPlayer.nextWeaponAttack - memory->globalVars->currenttime) / reloadLength), -IM_PI / 2, -IM_PI / 2 + IM_PI * 2), segments);
        const ImU32 color = Helpers::calculateColor(config->reloadProgress, memory->globalVars->realtime);
        drawList->PathStroke(color, false, config->reloadProgress.thickness);
    } else {
        reloadLength = 0.0f;
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
    const auto color = Helpers::calculateColor(config->recoilCrosshair, memory->globalVars->realtime);

    drawList->AddLine({ x, y - 10 }, { x, y + 10 }, color, config->recoilCrosshair.thickness);
    drawList->AddLine({ x - 10, y }, { x + 10, y }, color, config->recoilCrosshair.thickness);
}

void Misc::purchaseList(GameEvent* event) noexcept
{
    // TODO: collect only enemies' purchases
    static std::mutex mtx;
    std::scoped_lock _{ mtx };

    static std::unordered_map<std::string, std::vector<std::string>> purchases;
    static auto freezeEnd = 0.0f;

    if (event) {
        switch (fnv::hashRuntime(event->getName())) {
        case fnv::hash("item_purchase"): {
            std::string weapon = event->getString("weapon");
            if (weapon.starts_with("weapon_"))
                weapon.erase(0, 7);

            std::string playerName = "unknown";
            const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserId(event->getInt("userid")));
            if (player)
                playerName = player->getPlayerName(config->normalizePlayerNames);

            purchases[playerName].push_back(weapon);
            break;
        }
        case fnv::hash("round_start"):
            freezeEnd = 0.0f;
            purchases.clear();
            break;
        case fnv::hash("round_freeze_end"):
            freezeEnd = memory->globalVars->realtime;
            break;
        }
    } else {
        static auto mp_buytime = interfaces->cvar->findVar("mp_buytime");

        if (freezeEnd != 0.0f && memory->globalVars->realtime > freezeEnd + mp_buytime->getFloat())
            return;
        
        assert(gui);
        ImGui::Begin("Purchases", nullptr, gui->open ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoInputs);

        for (const auto& playerPurchases : purchases) {
            std::string s = std::accumulate(playerPurchases.second.begin(), playerPurchases.second.end(), std::string{ }, [](std::string s, const std::string& piece) { return s += piece + ", "; });

            ImGui::TextWrapped("%s: %s", playerPurchases.first.c_str(), s.c_str());
        }

        ImGui::End();
    }
}
