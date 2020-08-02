#pragma once

#include "VirtualMethod.h"

class Entity;

class EntityList {
public:
    VIRTUAL_METHOD(Entity*, getEntity, 3, (int index), (this, index))
#ifdef _WIN32
    VIRTUAL_METHOD(Entity*, getEntityFromHandle, 4, (int handle), (this, handle))
#elif __linux__
    VIRTUAL_METHOD(Entity*, getEntityFromHandle, 4, (int handle), (this, &handle))
#endif
    VIRTUAL_METHOD(int, getHighestEntityIndex, 6, (), (this))
};
