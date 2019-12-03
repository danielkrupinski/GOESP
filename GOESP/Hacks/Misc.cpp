#include "Misc.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

#include "../Config.h"
#include "../Helpers.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/GlobalVars.h"

void Misc::drawReloadProgress(ImDrawList* drawList) noexcept
{
    if (config.reloadProgress.enabled && interfaces.engine->isInGame()) {
        const auto localPlayer = interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer());

        if (!localPlayer)
            return;

        static float reloadLength = 0.0f;

        if (const auto activeWeapon = localPlayer->getActiveWeapon(); activeWeapon && activeWeapon->isInReload()) {
            if (!reloadLength)
                reloadLength = activeWeapon->nextPrimaryAttack() - memory.globalVars->currenttime;

            const auto [width, height] = interfaces.engine->getScreenSize();
            constexpr int segments = 20;
            drawList->PathArcTo({ width / 2.0f, height / 2.0f }, 20.0f, -IM_PI / 2, std::clamp(IM_PI * 2 * (0.75f - (activeWeapon->nextPrimaryAttack() - memory.globalVars->currenttime) / reloadLength), -IM_PI / 2, -IM_PI / 2 + IM_PI * 2), segments);
            const ImU32 color = Helpers::calculateColor(config.reloadProgress.color, config.reloadProgress.rainbow, config.reloadProgress.rainbowSpeed, memory.globalVars->realtime);
            drawList->PathStroke(color, false, 3.0f);
        } else {
            reloadLength = 0.0f;
        }
    }
}
