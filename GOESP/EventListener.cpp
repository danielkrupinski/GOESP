#include <cassert>

#include "EventListener.h"
#include "fnv.h"
#include "GameData.h"
#include "Hacks/Misc.h"
#include "Interfaces.h"
#include "SDK/GameEvent.h"

namespace
{
    class EventListenerImpl : public GameEventListener {
    public:
        void fireGameEvent(GameEvent* event)
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
    } impl;
}

void EventListener::init() noexcept
{
    assert(interfaces);

    const auto gameEventManager = interfaces->gameEventManager;
    gameEventManager->addListener(&impl, "round_start");
    gameEventManager->addListener(&impl, "round_freeze_end");
    gameEventManager->addListener(&impl, "player_hurt");
}

void EventListener::remove() noexcept
{
    assert(interfaces);

    interfaces->gameEventManager->removeListener(&impl);
}
