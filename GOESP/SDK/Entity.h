#pragma once

#include "ClientClass.h"
#include "EngineTrace.h"
#include "EntityList.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "Utils.h"
#include "WeaponId.h"

struct Vector;
struct WeaponInfo;
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

#define NETVAR(func_name, offset, type) \
std::add_lvalue_reference_t<type> func_name() noexcept \
{ \
    return *reinterpret_cast<std::add_pointer_t<type>>(this + offset); \
}

class Entity {
public:
    constexpr ClientClass* getClientClass() noexcept
    {
        return callVirtualMethod<ClientClass*, 2>(this + 8);
    }

    constexpr auto isDormant() noexcept
    {
        return callVirtualMethod<bool, 9>(this + 8);
    }

    constexpr auto getModel() noexcept
    {
        return callVirtualMethod<const Model*, 8>(this + 4);
    }

    constexpr auto getCollideable() noexcept
    {
        return callVirtualMethod<Collideable*, 3>(this);
    }

    constexpr auto getAbsOrigin() noexcept
    {
        return callVirtualMethod<Vector&, 10>(this);
    }

    constexpr auto isAlive() noexcept
    {
        return callVirtualMethod<bool, 155>(this);
    }

    constexpr auto isWeapon() noexcept
    {
        return callVirtualMethod<bool, 165>(this);
    }

    constexpr auto getActiveWeapon() noexcept
    {
        return callVirtualMethod<Entity*, 267>(this);
    }

    auto getEyePosition() noexcept
    {
        Vector vec;
        callVirtualMethod<void, 284>(this, std::ref(vec));
        return vec;
    }

    constexpr auto getObserverTarget() noexcept
    {
        return callVirtualMethod<Entity*, 294>(this);
    }

    constexpr auto getWeaponInfo() noexcept
    {
        return callVirtualMethod<WeaponInfo*, 459>(this);
    }

    bool visibleTo(Entity* other) noexcept
    {
        Trace trace;
        interfaces->engineTrace->traceRay({ other->getEyePosition(), getEyePosition() }, 0x46004009, other, trace);
        return trace.entity == this || trace.fraction > 0.97f;
    }

    NETVAR(weaponId, 0x2FAA, WeaponId)                                               // CBaseAttributableItem->m_iItemDefinitionIndex

    NETVAR(clip, 0x3254, int)                                                        // CBaseCombatWeapon->m_iClip1
    NETVAR(isInReload, 0x3254 + 0x41, bool)                                          // CBaseCombatWeapon->m_iClip1 + 0x41
    NETVAR(reserveAmmoCount, 0x325C, int)                                            // CBaseCombatWeapon->m_iPrimaryReserveAmmoCount
    NETVAR(nextPrimaryAttack, 0x3228, float)                                         // CBaseCombatWeapon->m_flNextPrimaryAttack

    NETVAR(index, 0x64, int)                                                         // CBaseEntity->m_bIsAutoaimTarget + 0x4
    NETVAR(ownerEntity, 0x14C, int)                                                  // CBaseEntity->m_hOwnerEntity

    NETVAR(aimPunchAngle, 0x302C, Vector)                                            // CBasePlayer->m_aimPunchAngle
    NETVAR(health, 0x100, int)                                                       // CBasePlayer->m_iHealth

    NETVAR(coordinateFrame, 0x444, Matrix3x4)
};
