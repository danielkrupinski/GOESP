#pragma once

#include "VirtualMethod.h"

class Entity;

class EntityList {
public:
    VIRTUAL_METHOD_V2(Entity*, getEntity, 3, (int index), (this, index))
    VIRTUAL_METHOD_V2(Entity*, getEntityFromHandle, 4, (int handle), (this, handle))
    VIRTUAL_METHOD_V2(int, getHighestEntityIndex, 6, (), (this))
};
