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

#define NETVAR_V2(func_name, var_name, offset, type) \
std::add_lvalue_reference_t<type> func_name() noexcept \
{ \
    return *reinterpret_cast<std::add_pointer_t<type>>(this + offset); \
}

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

    NETVAR_V2(weaponId, "CBaseAttributableItem->m_iItemDefinitionIndex", 0x2FAA, WeaponId);
    NETVAR_V2(clip, "CBaseCombatWeapon->m_iClip1", 0x3244, int);
    NETVAR_V2(reserveAmmoCount, "CBaseCombatWeapon->m_iPrimaryReserveAmmoCount", 0x324C, int);
    NETVAR_V2(nextPrimaryAttack, "CBaseCombatWeapon->m_flNextPrimaryAttack", 0x3218, float);
    NETVAR_V2(index, "CBaseEntity->m_bIsAutoaimTarget", 0x64, int);
    NETVAR_V2(ownerEntity, "CBaseEntity->m_hOwnerEntity", 0x14C, int);
    NETVAR_V2(aimPunchAngle, "CBasePlayer->m_aimPunchAngle", 0x302C, Vector);
    NETVAR_V2(health, "CBasePlayer->m_iHealth", 0x100, int);
};
