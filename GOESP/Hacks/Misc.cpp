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
#include "../SDK/ItemSchema.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/WeaponInfo.h"
#include "../SDK/WeaponSystem.h"

#include <mutex>
#include <numeric>
#include <unordered_map>
#include <vector>

struct LocalPlayerData {
    void update() noexcept
    {
        if (!localPlayer)
            return;

        exists = true;
        alive = localPlayer->isAlive();
        inBombZone = localPlayer->inBombZone();

        if (const auto activeWeapon = localPlayer->getActiveWeapon()) {
            inReload = activeWeapon->isInReload();
            shooting = localPlayer->shotsFired() > 0;

            if (const auto weaponInfo = activeWeapon->getWeaponInfo())
                fullAutoWeapon = weaponInfo->fullAuto;

            nextWeaponAttack = activeWeapon->nextPrimaryAttack();
        }
        aimPunch = localPlayer->getAimPunch();
    }
    bool exists = false;
    bool alive = false;
    bool inBombZone = false;
    bool inReload = false;
    bool shooting = false;
    bool fullAutoWeapon = false;
    float nextWeaponAttack = 0.0f;
    Vector aimPunch;
};

static LocalPlayerData localPlayerData;
static std::mutex dataMutex;

void Misc::collectData() noexcept
{
    std::scoped_lock _{ dataMutex };

    localPlayerData.update();
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

    if (!localPlayerData.fullAutoWeapon || !localPlayerData.shooting)
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

    static std::unordered_map<std::string, std::pair<std::vector<std::string>, int>> purchaseDetails;
    static std::unordered_map<std::string, int> purchaseTotal;
    static int totalCost;

    static auto freezeEnd = 0.0f;

    if (event) {
        switch (fnv::hashRuntime(event->getName())) {
        case fnv::hash("item_purchase"): {
            const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserId(event->getInt("userid")));

            if (player && localPlayer && memory->isOtherEnemy(player, localPlayer.get())) {
                if (const auto defintion = memory->itemSystem()->getItemSchema()->getItemDefinitionByName(event->getString("weapon"))) {
                    if (const auto weaponInfo = memory->weaponSystem->getWeaponInfo(defintion->getWeaponId())) {
                        purchaseDetails[player->getPlayerName(config->normalizePlayerNames)].second += weaponInfo->price;
                        totalCost += weaponInfo->price;
                    }
                }
                std::string weapon = event->getString("weapon");

                if (weapon.starts_with("weapon_"))
                    weapon.erase(0, 7);
                else if (weapon.starts_with("item_"))
                    weapon.erase(0, 5);

                if (weapon.starts_with("smoke"))
                    weapon = "smoke";
                else if (weapon.starts_with("m4a1_s"))
                    weapon = "m4a1_s";

                purchaseDetails[player->getPlayerName(config->normalizePlayerNames)].first.push_back(weapon);
                ++purchaseTotal[weapon];
            }
            break;
        }
        case fnv::hash("round_start"):
            freezeEnd = 0.0f;
            purchaseDetails.clear();
            purchaseTotal.clear();
            totalCost = 0;
            break;
        case fnv::hash("round_freeze_end"):
            freezeEnd = memory->globalVars->realtime;
            break;
        }
    } else {
        if (!config->purchaseList.enabled)
            return;

        static const auto mp_buytime = interfaces->cvar->findVar("mp_buytime");

        if ((!interfaces->engine->isInGame() || freezeEnd != 0.0f && memory->globalVars->realtime > freezeEnd + (!config->purchaseList.onlyDuringFreezeTime ? mp_buytime->getFloat() : 0.0f) || purchaseDetails.empty() || purchaseTotal.empty()) && !gui->open)
            return;
        
        ImGui::SetNextWindowSize({ 200.0f, 200.0f }, ImGuiCond_Once);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
        if (!gui->open)
            windowFlags |= ImGuiWindowFlags_NoInputs;
        if (config->purchaseList.noTitleBar)
            windowFlags |= ImGuiWindowFlags_NoTitleBar;
        
        ImGui::Begin("Purchases", nullptr, windowFlags);
        
        if (config->purchaseList.mode == PurchaseList::Details) {
            for (const auto& [playerName, purchases] : purchaseDetails) {
                std::string s = std::accumulate(purchases.first.begin(), purchases.first.end(), std::string{ }, [](std::string s, const std::string& piece) { return s += piece + ", "; });
                if (s.length() >= 2)
                    s.erase(s.length() - 2);

                if (config->purchaseList.showPrices)
                    ImGui::TextWrapped("%s $%d: %s", playerName.c_str(), purchases.second, s.c_str());
                else
                    ImGui::TextWrapped("%s: %s", playerName.c_str(), s.c_str());
            }
        } else if (config->purchaseList.mode == PurchaseList::Summary) {
            for (const auto& purchase : purchaseTotal)
                ImGui::TextWrapped("%d x %s", purchase.second, purchase.first.c_str());

            if (config->purchaseList.showPrices && totalCost > 0) {
                ImGui::Separator();
                ImGui::TextWrapped("Total: $%d", totalCost);
            }
        }
        ImGui::End();
    }
}

void Misc::drawBombZoneHint() noexcept
{
    if (!config->bombZoneHint)
        return;

    std::scoped_lock _{ dataMutex };

    if (!localPlayerData.exists || !localPlayerData.alive)
        return;

    if (!gui->open && !localPlayerData.inBombZone)
        return;

    ImGui::SetNextWindowSize({});
    ImGui::Begin("Bomb Zone Hint", nullptr, ImGuiWindowFlags_NoDecoration | (gui->open ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoInputs));
    ImGui::TextUnformatted("You're in bomb zone!");
    ImGui::End();
}
