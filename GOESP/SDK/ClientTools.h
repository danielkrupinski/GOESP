#pragma once

#include "Inconstructible.h"
#include "VirtualMethod.h"

class Entity;

class ClientTools {
public:
    INCONSTRUCTIBLE(ClientTools)

    VIRTUAL_METHOD_V(Entity*, nextEntity, 7, (Entity* entity), (this, entity))
};
