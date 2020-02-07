#pragma once

#include "ClientClass.h"
#include "EngineTrace.h"
#include "EntityList.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "VirtualMethod.h"
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
    VIRTUAL_METHOD_(getClientClass, ClientClass*, 2, this + 8)
    VIRTUAL_METHOD_(isDormant, bool, 9, this + 8)
    VIRTUAL_METHOD_(getModel, const Model*, 8, this + 4)

    VIRTUAL_METHOD(getCollideable, Collideable*, 3)
    VIRTUAL_METHOD(getAbsOrigin, Vector&, 10)
    VIRTUAL_METHOD(isAlive, bool, 155)
    VIRTUAL_METHOD(isWeapon, bool, 165)
    VIRTUAL_METHOD(getActiveWeapon, Entity*, 267)
    VIRTUAL_METHOD(getObserverTarget, Entity*, 294)
    VIRTUAL_METHOD(getWeaponInfo, WeaponInfo*, 459)
   
    auto getEyePosition() noexcept
    {
        Vector vec;
        VirtualMethod::call<void, 284>(this, std::ref(vec));
        return vec;
    }

    bool visibleTo(Entity* other) noexcept
    {
        Trace trace;
        interfaces->engineTrace->traceRay({ other->getEyePosition(), getEyePosition() }, 0x46004009, other, trace);
        return trace.entity == this || trace.fraction > 0.97f;
    }

    auto getAimPunch() noexcept
    {
        Vector vec;
        VirtualMethod::call<void, 345>(this, std::ref(vec));
        return vec;
    }

    NETVAR(weaponId, 0x2FAA, WeaponId)                                               // CBaseAttributableItem->m_iItemDefinitionIndex

    NETVAR(clip, 0x3254, int)                                                        // CBaseCombatWeapon->m_iClip1
    NETVAR(isInReload, 0x3254 + 0x41, bool)                                          // CBaseCombatWeapon->m_iClip1 + 0x41
    NETVAR(reserveAmmoCount, 0x325C, int)                                            // CBaseCombatWeapon->m_iPrimaryReserveAmmoCount
    NETVAR(nextPrimaryAttack, 0x3228, float)                                         // CBaseCombatWeapon->m_flNextPrimaryAttack

    NETVAR(index, 0x64, int)                                                         // CBaseEntity->m_bIsAutoaimTarget + 0x4
    NETVAR(ownerEntity, 0x14C, int)                                                  // CBaseEntity->m_hOwnerEntity

    NETVAR(health, 0x100, int)                                                       // CBasePlayer->m_iHealth

    NETVAR(coordinateFrame, 0x444, Matrix3x4)
};
