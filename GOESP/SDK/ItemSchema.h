#pragma once

#include "VirtualMethod.h"
#include "WeaponId.h"

class EconItemDefintion {
public:
    VIRTUAL_METHOD_V2(WeaponId, getWeaponId, 0, (), (this))
};

class ItemSchema {
public:
    VIRTUAL_METHOD_V2(EconItemDefintion*, getItemDefinitionByName, 42, (const char* name), (this, name))
};

class ItemSystem {
public:
    VIRTUAL_METHOD_V2(ItemSchema*, getItemSchema, 0, (), (this))
};
