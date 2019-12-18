#pragma once

#include "ClientClass.h"
#include "EngineTrace.h"
#include "EntityList.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"
#include "Utils.h"
#include "WeaponId.h"

struct Vector;
struct WeaponData;
class Matrix3x4;

class Collideable {
virtual void* pad() = 0;
public:
    virtual const Vector& obbMins() = 0;
    virtual const Vector& obbMaxs() = 0;
};

struct Model {
    void* handle;
    char name[260];
};

class Entity {
public:
    constexpr ClientClass* getClientClass() noexcept
    {
        return callVirtualMethod<ClientClass*>(this + 8, 2);
    }

    constexpr auto isDormant() noexcept
    {
        return callVirtualMethod<bool>(this + 8, 9);
    }

    constexpr auto getModel() noexcept
    {
        return callVirtualMethod<const Model*>(this + 4, 8);
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

    constexpr auto getActiveWeapon() noexcept
    {
        return callVirtualMethod<Entity*>(this, 267);
    }

    auto getEyePosition() noexcept
    {
        Vector vec;
        callVirtualMethod<void, Vector&>(this, 284, vec);
        return vec;
    }

    constexpr auto getObserverTarget() noexcept
    {
        return callVirtualMethod<Entity*>(this, 294);
    }

    constexpr auto getWeaponData() noexcept
    {
        return callVirtualMethod<WeaponData*>(this, 457);
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

    auto isInReload() noexcept
    {
        return *reinterpret_cast<bool*>(uintptr_t(&clip()) + 0x41);
    }

    NETVAR(weaponId, "CBaseAttributableItem", "m_iItemDefinitionIndex", WeaponId);

    NETVAR(clip, "CBaseCombatWeapon", "m_iClip1", int);
    NETVAR(reserveAmmoCount, "CBaseCombatWeapon", "m_iPrimaryReserveAmmoCount", int);
    NETVAR(nextPrimaryAttack, "CBaseCombatWeapon", "m_flNextPrimaryAttack", float);

    NETVAR_OFFSET(index, "CBaseEntity", "m_bIsAutoaimTarget", 4, int);
    NETVAR(ownerEntity, "CBaseEntity", "m_hOwnerEntity", int);

    NETVAR(aimPunchAngle, "CBasePlayer", "m_aimPunchAngle", Vector);
    NETVAR(health, "CBasePlayer", "m_iHealth", int);
};
