#include <algorithm>
#include <cstring>

#include "Engine.h"
#include "EngineTrace.h"
#include "Entity.h"
#include "GlobalVars.h"
#include "../Interfaces.h"
#include "Localize.h"
#include "LocalPlayer.h"
#include "../Memory.h"
#include "ModelInfo.h"
#include "PlayerResource.h"

bool CSPlayer::canSee(Entity* other, const Vector& pos) noexcept
{
    const auto eyePos = getEyePosition();

    if (memory->lineGoesThroughSmoke(eyePos, pos, 1))
        return false;

    Trace trace;
    interfaces->engineTrace->traceRay({ eyePos, pos }, 0x46004009, this, trace);
    return trace.entity == other || trace.fraction > 0.97f;
}

bool CSPlayer::visibleTo(CSPlayer* other) noexcept
{
    assert(isAlive());

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

    for (const auto boxNum : { Hitbox::Belly, Hitbox::LeftForearm, Hitbox::RightForearm }) {
        const auto hitbox = set->getHitbox(boxNum);
        if (hitbox && other->canSee(this, boneMatrices[hitbox->bone].origin()))
            return true;
    }

    return false;
}

[[nodiscard]] std::string CSPlayer::getPlayerName() noexcept
{
    char name[128];
    getPlayerName(name);
    return name;
}

void CSPlayer::getPlayerName(char(&out)[128]) noexcept
{
    if (!*memory->playerResource) {
        strcpy(out, "unknown");
        return;
    }

    wchar_t wide[128];
    memory->getDecoratedPlayerName(*memory->playerResource, index(), wide, sizeof(wide), 4);

    auto end = std::remove(wide, wide + wcslen(wide), L'\n');
    *end = L'\0';
    end = std::unique(wide, end, [](wchar_t a, wchar_t b) { return a == L' ' && a == b; });
    *end = L'\0';

    interfaces->localize->convertUnicodeToAnsi(wide, out, 128);
}

int CSPlayer::getUserId() noexcept
{
    if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(index(), playerInfo))
        return playerInfo.userId;
    return -1;
}

bool CSPlayer::isEnemy() noexcept
{
    if (!localPlayer) {
        assert(false);
        return false;
    }

    if (const auto localTeam = localPlayer->getTeamNumber(); localTeam != Team::TT && localTeam != Team::CT) {
        // if we're in Spectators team treat CTs as allies and TTs as enemies
        return getTeamNumber() != Team::CT;
    }
    return memory->isOtherEnemy(this, localPlayer.get());
}

bool CSPlayer::isGOTV() noexcept
{
    if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(index(), playerInfo))
        return playerInfo.hltv;
    return false;
}

std::uint64_t CSPlayer::getSteamID() noexcept
{
    if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(index(), playerInfo))
        return playerInfo.xuid;
    return 0;
}
