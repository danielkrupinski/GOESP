#pragma once

#include "VirtualMethod.h"
#include "WeaponId.h"

class EconItemDefintion {
public:
    INCONSTRUCTIBLE(EconItemDefintion)

    VIRTUAL_METHOD(WeaponId, getWeaponId, 0, (), (this))
    VIRTUAL_METHOD(const char*, getItemBaseName, 2, (), (this))
};

class ItemSchema {
public:
    INCONSTRUCTIBLE(ItemSchema)

    VIRTUAL_METHOD(EconItemDefintion*, getItemDefinitionByName, 42, (const char* name), (this, name))
};

class ItemSystem {
public:
    INCONSTRUCTIBLE(ItemSystem)

    VIRTUAL_METHOD(ItemSchema*, getItemSchema, 0, (), (this))
};
