#include <cassert>

#include "EventListener.h"
#include "fnv.h"
#include "GameData.h"
#include "Hacks/Misc.h"
#include "Interfaces.h"

EventListener::EventListener() noexcept
{
    assert(interfaces);

    const auto gameEventManager = interfaces->gameEventManager;
    gameEventManager->addListener(this, "round_start");
    gameEventManager->addListener(this, "round_freeze_end");
    gameEventManager->addListener(this, "player_hurt");
}

void EventListener::remove() noexcept
{
    assert(interfaces);

    interfaces->gameEventManager->removeListener(this);
}

void EventListener::fireGameEvent(GameEvent* event)
{
    switch (fnv::hashRuntime(event->getName())) {
    case fnv::hash("round_start"):
        GameData::clearProjectileList();
        GameData::clearPlayersLastLocation();
        [[fallthrough]];
    case fnv::hash("round_freeze_end"):
        Misc::purchaseList(event);
        break;
    case fnv::hash("player_hurt"):
        Misc::hitEffect(*event);
        break;
    }
}
