#pragma once

#include "VirtualMethod.h"

class Entity;

class EntityList {
public:
    constexpr auto getEntity(int index) noexcept
    {
        return VirtualMethod::call<Entity*, 3>(this, index);
    }

    constexpr auto getEntityFromHandle(int handle) noexcept
    {
        return VirtualMethod::call<Entity*, 4>(this, handle);
    }

    constexpr auto getHighestEntityIndex() noexcept
    {
        return VirtualMethod::call<int, 6>(this);
    }
};
