#pragma once

#include "VirtualMethod.h"

class Entity;

class EntityList {
public:
    constexpr auto getEntity(int index) noexcept
    {
        return callVirtualMethod<Entity*, 3>(this, index);
    }

    constexpr auto getEntityFromHandle(int handle) noexcept
    {
        return callVirtualMethod<Entity*, 4>(this, handle);
    }

    constexpr auto getHighestEntityIndex() noexcept
    {
        return callVirtualMethod<int, 6>(this);
    }
};
