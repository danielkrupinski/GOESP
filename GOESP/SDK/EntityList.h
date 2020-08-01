#pragma once

#include "VirtualMethod.h"

class Entity;

class EntityList {
public:
#ifdef _WIN32
    VIRTUAL_METHOD(Entity*, getEntity, 3, (int index), (this, index))
    VIRTUAL_METHOD(Entity*, getEntityFromHandle, 4, (int handle), (this, handle))
    VIRTUAL_METHOD(int, getHighestEntityIndex, 6, (), (this))
#elif __linux__
    VIRTUAL_METHOD(Entity*, getEntity, 6, (int index), (this, index))
    VIRTUAL_METHOD(Entity*, getEntityFromHandle, 10, (int handle), (this, handle))
    VIRTUAL_METHOD(int, getHighestEntityIndex, 11, (), (this))
#endif
};
