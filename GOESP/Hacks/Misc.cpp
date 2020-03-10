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
#include "../SDK/WeaponInfo.h"

#include <mutex>
#include <numeric>
#include <unordered_map>
#include <vector>

struct LocalPlayerData {
    LocalPlayerData() = default;
    LocalPlayerData(Entity* localPlayer) noexcept
    {
        if (!localPlayer)
            return;

        exists = true;
        alive = localPlayer->isAlive();

        if (const auto activeWeapon = localPlayer->getActiveWeapon()) {
            inReload = activeWeapon->isInReload();
            if (const auto weaponInfo = activeWeapon->getWeaponInfo())
                fullAutoWeapon = weaponInfo->fullAuto;

           nextWeaponAttack = activeWeapon->nextPrimaryAttack();
        }
        aimPunch = localPlayer->getAimPunch();
    }
    bool exists = false;
    bool alive = false;
    bool inReload = false;
    bool fullAutoWeapon = false;
    float nextWeaponAttack = 0.0f;
    Vector aimPunch;
};

static LocalPlayerData localPlayerData;
static std::mutex dataMutex;

void Misc::collectData() noexcept
{
    std::scoped_lock _{ dataMutex };

    localPlayerData = { interfaces->entityList->getEntity(interfaces->engine->getLocalPlayer()) };
}

void Misc::drawReloadProgress(ImDrawList* drawList) noexcept
{
    if (!config->reloadProgress.enabled)
        return;

    std::scoped_lock _{ dataMutex };

    if (!localPlayerData.exists || !localPlayerData.alive)
        return;

    static float reloadLength = 0.0f;

    if (localPlayerData.inReload) {
        if (!reloadLength)
            reloadLength = localPlayerData.nextWeaponAttack - memory->globalVars->currenttime;

        const auto [width, height] = interfaces->engine->getScreenSize();
        constexpr int segments = 20;
        drawList->PathArcTo({ width / 2.0f, height / 2.0f }, 20.0f, -IM_PI / 2, std::clamp(IM_PI * 2 * (0.75f - (localPlayerData.nextWeaponAttack - memory->globalVars->currenttime) / reloadLength), -IM_PI / 2, -IM_PI / 2 + IM_PI * 2), segments);
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

    if (!localPlayerData.exists || !localPlayerData.alive)
        return;

    if (!localPlayerData.fullAutoWeapon)
        return;

    const auto [width, height] = interfaces->engine->getScreenSize();

    const float x = width * (0.5f - localPlayerData.aimPunch.y / 180.0f);
    const float y = height * (0.5f + localPlayerData.aimPunch.x / 180.0f);
    const auto color = Helpers::calculateColor(config->recoilCrosshair, memory->globalVars->realtime);

    drawList->AddLine({ x, y - 10 }, { x, y + 10 }, color, config->recoilCrosshair.thickness);
    drawList->AddLine({ x - 10, y }, { x + 10, y }, color, config->recoilCrosshair.thickness);
}

void Misc::purchaseList(GameEvent* event) noexcept
{
    static std::mutex mtx;
    std::scoped_lock _{ mtx };

    static std::unordered_map<std::string, std::vector<std::string>> purchaseDetails;
    static std::unordered_map<std::string, int> purchaseTotal;

    static auto freezeEnd = 0.0f;

    if (event) {
        switch (fnv::hashRuntime(event->getName())) {
        case fnv::hash("item_purchase"): {
            std::string weapon = event->getString("weapon");
            if (weapon.starts_with("weapon_"))
                weapon.erase(0, 7);
            else if (weapon.starts_with("item_"))
                weapon.erase(0, 5);

            const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserId(event->getInt("userid")));
            const auto localPlayer = interfaces->entityList->getEntity(interfaces->engine->getLocalPlayer());

            if (player && localPlayer && memory->isOtherEnemy(player, localPlayer)) {
                purchaseDetails[player->getPlayerName(config->normalizePlayerNames)].push_back(weapon);
                ++purchaseTotal[weapon];
            }
            break;
        }
        case fnv::hash("round_start"):
            freezeEnd = 0.0f;
            purchaseDetails.clear();
            purchaseTotal.clear();
            break;
        case fnv::hash("round_freeze_end"):
            freezeEnd = memory->globalVars->realtime;
            break;
        }
    } else {
        if (!config->purchaseList)
            return;

        static auto mp_buytime = interfaces->cvar->findVar("mp_buytime");

        if ((!interfaces->engine->isInGame() || freezeEnd != 0.0f && memory->globalVars->realtime > freezeEnd + mp_buytime->getFloat()) && !gui->open)
            return;
        
        ImGui::SetNextWindowSize({ 100.0f, 100.0f }, ImGuiCond_Once);
        ImGui::Begin("Purchases", nullptr, ImGuiWindowFlags_NoCollapse | (gui->open ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoInputs));

        if (config->purchaseListMode == 0) {
            for (const auto& playerPurchases : purchaseDetails) {
                std::string s = std::accumulate(playerPurchases.second.begin(), playerPurchases.second.end(), std::string{ }, [](std::string s, const std::string& piece) { return s += piece + ", "; });
                if (s.length() >= 2)
                    s.erase(s.length() - 2);
                ImGui::TextWrapped("%s: %s", playerPurchases.first.c_str(), s.c_str());
            }
        } else if (config->purchaseListMode == 1) {
            for (const auto& purchase : purchaseTotal) {
                ImGui::TextWrapped("%d x %s", purchase.second, purchase.first.c_str());
            }
        }
        ImGui::End();
    }
}
