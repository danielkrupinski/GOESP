#pragma once

#include "VirtualMethod.h"
#include "WeaponId.h"

class EconItemDefintion {
public:
    VIRTUAL_METHOD(getWeaponId, WeaponId, 0)
};

class ItemSchema {
public:
    constexpr auto getItemDefinitionByName(const char* name) noexcept
    {
        return VirtualMethod::call<EconItemDefintion*, 42>(this, name);
    }
};

class ItemSystem {
public:
    VIRTUAL_METHOD(getItemSchema, ItemSchema*, 0)
};
