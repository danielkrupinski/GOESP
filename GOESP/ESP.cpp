#include "ESP.h"

#include "imgui/imgui.h"

#include "Interfaces.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"

void ESP::render(ImDrawList* drawList) noexcept
{
    if (interfaces.engine->isInGame()) {
        const auto localPlayer = interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer());

        if (!localPlayer)
            return;

        for (int i = 1; i <= interfaces.engine->getMaxClients(); i++) {
            const auto entity = interfaces.entityList->getEntity(i);
            if (!entity || entity == localPlayer || entity->isDormant() || !entity->isAlive())
                continue;


        }
        if (localPlayer->health() != 0)
        drawList->AddCircle({ 300, 300 }, 15.0f, 0xFF00FF00);
    }
    drawList->AddCircle({ 200, 200 }, 15.0f, 0xFFFF0000);
}
