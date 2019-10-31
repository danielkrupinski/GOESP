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

static void renderBox(ImDrawList* drawList, Entity* entity, const BoundingBox& bbox, const Config::Shared& config) noexcept
{
    if (config.box.enabled) {
        ImU32 color = config.box.rainbow ? ImGui::ColorConvertFloat4ToU32(rainbowColor(10.0f, config.box.rainbowSpeed)) : ImGui::ColorConvertFloat4ToU32(config.box.color);

        switch (config.boxType) {
        case 0:
            drawList->AddRect({ bbox.x0, bbox.y0 }, { bbox.x1, bbox.y1 }, color);
            break;
            /*
        case 1:
            /*
            drawList->AddLine({ bbox.x0, bbox.y0 }, { bbox.x1, bbox.y1 }, color);

            interfaces.surface->drawLine(bbox.x0, bbox.y0, bbox.x0, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces.surface->drawLine(bbox.x0, bbox.y0, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0);
            interfaces.surface->drawLine(bbox.x1, bbox.y0, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0);
            interfaces.surface->drawLine(bbox.x1, bbox.y0, bbox.x1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces.surface->drawLine(bbox.x0, bbox.y1, bbox.x0, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces.surface->drawLine(bbox.x0, bbox.y1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1);
            interfaces.surface->drawLine(bbox.x1, bbox.y1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1);
            interfaces.surface->drawLine(bbox.x1, bbox.y1, bbox.x1, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);
            
            break;
        case 2:
            for (int i = 0; i < 8; i++) {
                if (!(i & 1))
                    interfaces.surface->drawLine(bbox.vertices[i].x, bbox.vertices[i].y, bbox.vertices[i + 1].x, bbox.vertices[i + 1].y);
                if (!(i & 2))
                    interfaces.surface->drawLine(bbox.vertices[i].x, bbox.vertices[i].y, bbox.vertices[i + 2].x, bbox.vertices[i + 2].y);
                if (!(i & 4))
                    interfaces.surface->drawLine(bbox.vertices[i].x, bbox.vertices[i].y, bbox.vertices[i + 4].x, bbox.vertices[i + 4].y);
            }
            break;
        case 3:
            for (int i = 0; i < 8; i++) {
                if (!(i & 1)) {
                    interfaces.surface->drawLine(bbox.vertices[i].x, bbox.vertices[i].y, bbox.vertices[i].x + (bbox.vertices[i + 1].x - bbox.vertices[i].x) * 0.25f, bbox.vertices[i].y + (bbox.vertices[i + 1].y - bbox.vertices[i].y) * 0.25f);
                    interfaces.surface->drawLine(bbox.vertices[i].x + (bbox.vertices[i + 1].x - bbox.vertices[i].x) * 0.75f, bbox.vertices[i].y + (bbox.vertices[i + 1].y - bbox.vertices[i].y) * 0.75f, bbox.vertices[i + 1].x, bbox.vertices[i + 1].y);
                }
                if (!(i & 2)) {
                    interfaces.surface->drawLine(bbox.vertices[i].x, bbox.vertices[i].y, bbox.vertices[i].x + (bbox.vertices[i + 2].x - bbox.vertices[i].x) * 0.25f, bbox.vertices[i].y + (bbox.vertices[i + 2].y - bbox.vertices[i].y) * 0.25f);
                    interfaces.surface->drawLine(bbox.vertices[i].x + (bbox.vertices[i + 2].x - bbox.vertices[i].x) * 0.75f, bbox.vertices[i].y + (bbox.vertices[i + 2].y - bbox.vertices[i].y) * 0.75f, bbox.vertices[i + 2].x, bbox.vertices[i + 2].y);
                }
                if (!(i & 4)) {
                    interfaces.surface->drawLine(bbox.vertices[i].x, bbox.vertices[i].y, bbox.vertices[i].x + (bbox.vertices[i + 4].x - bbox.vertices[i].x) * 0.25f, bbox.vertices[i].y + (bbox.vertices[i + 4].y - bbox.vertices[i].y) * 0.25f);
                    interfaces.surface->drawLine(bbox.vertices[i].x + (bbox.vertices[i + 4].x - bbox.vertices[i].x) * 0.75f, bbox.vertices[i].y + (bbox.vertices[i + 4].y - bbox.vertices[i].y) * 0.75f, bbox.vertices[i + 4].x, bbox.vertices[i + 4].y);
                }
            }
            break;
            */
        }
    }
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
                renderBox(drawList, entity, bbox, config.players[0]);
            }

        }
    }
    const auto [width, height] = interfaces.engine->getScreenSize();

    drawList->AddCircle({ float(width / 2), float(height / 2) }, 15.0f, ImGui::ColorConvertFloat4ToU32(config.players[0].box.color));
}
