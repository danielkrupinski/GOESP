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

    constexpr auto getAbsOrigin() noexcept
    {
        return callVirtualMethod<Vector&>(this, 10);
    }

    constexpr auto isAlive() noexcept
    {
        return callVirtualMethod<bool>(this, 155) && health() > 0;
    }

    constexpr auto isWeapon() noexcept
    {
        return callVirtualMethod<bool>(this, 165);
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

    bool isVisible() noexcept
    {
        const auto localPlayer = interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer());

        Trace trace;
        interfaces.engineTrace->traceRay({ localPlayer->getEyePosition(), getEyePosition() }, 0x46004009, localPlayer, trace);
        return trace.entity == this || trace.fraction > 0.97f;
    }

    NETVAR(health, "CBasePlayer", "m_iHealth", int);
};
