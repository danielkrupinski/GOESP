#pragma once

#include "VirtualMethod.h"
#include "WeaponId.h"

class EconItemDefintion {
public:
    VIRTUAL_METHOD(WeaponId, getWeaponId, 0, (), (this))
};

class ItemSchema {
public:
    VIRTUAL_METHOD(EconItemDefintion*, getItemDefinitionByName, 42, (const char* name), (this, name))
};

class ItemSystem {
public:
    VIRTUAL_METHOD(ItemSchema*, getItemSchema, 0, (), (this))
};
