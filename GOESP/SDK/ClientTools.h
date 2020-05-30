#pragma once

#include "VirtualMethod.h"

class Entity;

class ClientTools {
public:
    VIRTUAL_METHOD(Entity*, nextEntity, 7, (Entity* entity), (this, entity))
};
