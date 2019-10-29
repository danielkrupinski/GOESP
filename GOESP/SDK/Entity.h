#pragma once

#include "../Netvars.h"
#include "Utils.h"

class Entity {
public:
    constexpr bool isDormant() noexcept
    {
        return callVirtualMethod<bool>(this + 8, 9);
    }

    constexpr bool isAlive() noexcept
    {
        return callVirtualMethod<bool>(this, 155) && health() > 0;
    }

    NETVAR(health, "CBasePlayer", "m_iHealth", int);
};
