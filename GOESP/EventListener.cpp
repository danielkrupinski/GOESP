#include <cassert>

#include "EventListener.h"
#include "Interfaces.h"
#include "Memory.h"

EventListener::EventListener() noexcept
{
    assert(intefaces);

    interfaces->gameEventManager->addListener(this, "item_purchase");
}

EventListener::~EventListener()
{
    assert(intefaces);

    interfaces->gameEventManager->removeListener(this);
}

void EventListener::fireGameEvent(GameEvent* event)
{
    assert(memory);

  //  if (std::strcmp(event->getName(), "item_purchase") == 0) {
  //      memory->debugMsg("ItemPurchased: %s\n", event->getString("weapon"));
  //  }
}
