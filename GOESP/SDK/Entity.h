#pragma once

#include "EngineTrace.h"
#include "EntityList.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"
#include "Utils.h"

struct Vector;
class Matrix3x4;

class Collideable {
public:
    virtual void* pad() = 0;
    virtual const Vector& obbMins() = 0;
    virtual const Vector& obbMaxs() = 0;
};

class Entity {
public:
    constexpr auto isDormant() noexcept
    {
        return callVirtualMethod<bool>(this + 8, 9);
    }

    constexpr auto getCollideable() noexcept
    {
        return callVirtualMethod<Collideable*>(this, 3);
    }

    constexpr auto isAlive() noexcept
    {
        return callVirtualMethod<bool>(this, 155) && health() > 0;
    }

    constexpr auto getEyePosition() noexcept
    {
        Vector vec{ };
        callVirtualMethod<void, Vector&>(this, 283, vec);
        return vec;
    }

    auto& coordinateFrame() noexcept
    {
        return *reinterpret_cast<Matrix3x4*>(this + 0x444);
    }

    bool isEnemy() noexcept
    {
        return memory.isOtherEnemy(this, interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer()));
    }
    NETVAR(health, "CBasePlayer", "m_iHealth", int);
};
