#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "EngineTrace.h"
#include "Entity.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "ModelInfo.h"
#include "GlobalVars.h"
#include "Engine.h"
#include "Localize.h"

bool Entity::canSee(Entity* other, const Vector& pos) noexcept
{
    Trace trace;
    interfaces->engineTrace->traceRay({ getEyePosition(), pos }, 0x46004009, this, trace);
    return (trace.entity == other || trace.fraction > 0.97f) && !memory->lineGoesThroughSmoke(getEyePosition(), pos, 1);
}

bool Entity::visibleTo(Entity* other) noexcept
{
    if (other->canSee(this, getAbsOrigin() + Vector{ 0.0f, 0.0f, 5.0f }))
        return true;

    if (other->canSee(this, getEyePosition() + Vector{ 0.0f, 0.0f, 5.0f }))
        return true;

    const auto model = getModel();
    if (!model)
        return false;

    const auto studioModel = interfaces->modelInfo->getStudioModel(model);
    if (!studioModel)
        return false;

    const auto set = studioModel->getHitboxSet(hitboxSet());
    if (!set)
        return false;

    Matrix3x4 boneMatrices[MAXSTUDIOBONES];
    if (!setupBones(boneMatrices, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, memory->globalVars->currenttime))
        return false;

    for (const auto boxNum : { 9, 14, 17 }) { // guts, left & right elbow hitbox
        if (boxNum < set->numHitboxes && other->canSee(this, boneMatrices[set->getHitbox(boxNum)->bone].origin()))
            return true;
    }

    return false;
}

[[nodiscard]] std::string Entity::getPlayerName() noexcept
{
    char name[128];
    getPlayerName(name);
    return name;
}

void Entity::getPlayerName(char(&out)[128]) noexcept
{
    PlayerInfo playerInfo;
    if (!interfaces->engine->getPlayerInfo(index(), playerInfo)) {
        strcpy(out, "unknown");
        return;
    }

    auto end = std::remove(playerInfo.name, playerInfo.name + strlen(playerInfo.name), '\n');
    *end = '\0';
    end = std::unique(playerInfo.name, end, [](char a, char b) { return a == b && a == ' '; });
    *end = '\0';

#ifdef _WIN32
    wchar_t wide[128];
    interfaces->localize->convertAnsiToUnicode(playerInfo.name, wide, sizeof(wide));
    wchar_t wideNormalized[128];
    NormalizeString(NormalizationKC, wide, -1, wideNormalized, 128);
    interfaces->localize->convertUnicodeToAnsi(wideNormalized, playerInfo.name, 128);
#endif

    strcpy(out, playerInfo.name);
}
