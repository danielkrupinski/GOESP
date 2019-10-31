#include "ESP.h"

#include "imgui/imgui.h"

#include "Config.h"
#include "Interfaces.h"
#include "SDK/DebugOverlay.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/Utils.h"
#include "SDK/Vector.h"

struct BoundingBox {
    float x0, y0;
    float x1, y1;
    Vector vertices[8];
};

static auto boundingBox(Entity* entity, BoundingBox& out) noexcept
{
    const auto [width, height] = interfaces.engine->getScreenSize();

    out.x0 = static_cast<float>(width * 2);
    out.y0 = static_cast<float>(height * 2);
    out.x1 = -static_cast<float>(width * 2);
    out.y1 = -static_cast<float>(height * 2);

    const auto mins = entity->getCollideable()->obbMins();
    const auto maxs = entity->getCollideable()->obbMaxs();

    for (int i = 0; i < 8; i++) {
        const Vector point{ i & 1 ? maxs.x : mins.x,
                            i & 2 ? maxs.y : mins.y,
                            i & 4 ? maxs.z : mins.z };

        if (interfaces.debugOverlay->screenPosition(point.transform(entity->coordinateFrame()), out.vertices[i]))
            return false;

        if (out.x0 > out.vertices[i].x)
            out.x0 = out.vertices[i].x;

        if (out.y0 > out.vertices[i].y)
            out.y0 = out.vertices[i].y;

        if (out.x1 < out.vertices[i].x)
            out.x1 = out.vertices[i].x;

        if (out.y1 < out.vertices[i].y)
            out.y1 = out.vertices[i].y;
    }
    return true;
}

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

            if (BoundingBox bbox; boundingBox(entity, bbox)) {

            }

        }
    }
    const auto [width, height] = interfaces.engine->getScreenSize();

    drawList->AddCircle({ float(width / 2), float(height / 2) }, 15.0f, packColor(config.players[0].box.color));
}
