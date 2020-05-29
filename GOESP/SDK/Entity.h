#pragma once

#include <string>

#include "Vector.h"
#include "VirtualMethod.h"

struct ClientClass;
enum class WeaponId : short;
struct WeaponInfo;
class Matrix3x4;

class Collideable {
public:
    VIRTUAL_METHOD(const Vector&, obbMins, 1, (), (this))
    VIRTUAL_METHOD(const Vector&, obbMaxs, 2, (), (this))
};

struct Model {
    void* handle;
    char name[260];
    int	loadFlags;
    int	serverCount;
    int	type;
    int	flags;
    Vector mins, maxs;
};

#define PROP(func_name, offset, type) \
std::add_lvalue_reference_t<std::add_const_t<type>> func_name() noexcept \
{ \
    return *reinterpret_cast<std::add_pointer_t<type>>(this + offset); \
}

enum class ObsMode {
    None = 0,
    Deathcam,
    Freezecam,
    Fixed,
    InEye,
    Chase,
    Roaming
};

class Entity {
public:
    VIRTUAL_METHOD(ClientClass*, getClientClass, 2, (), (this + 8))
    VIRTUAL_METHOD(bool, isDormant, 9, (), (this + 8))
    VIRTUAL_METHOD(int, index, 10, (), (this + 8))

    VIRTUAL_METHOD(const Model*, getModel, 8, (), (this + 4))
    VIRTUAL_METHOD(bool, setupBones, 13, (Matrix3x4* out, int maxBones, int boneMask, float currentTime), (this + 4, out, maxBones, boneMask, currentTime))
    VIRTUAL_METHOD(const Matrix3x4&, toWorldTransform, 32, (), (this + 4))

    VIRTUAL_METHOD(int&, handle, 2, (), (this))
    VIRTUAL_METHOD(Collideable*, getCollideable, 3, (), (this))
    VIRTUAL_METHOD(Vector&, getAbsOrigin, 10, (), (this))
    VIRTUAL_METHOD(bool, isAlive, 155, (), (this))
    VIRTUAL_METHOD(bool, isPlayer, 157, (), (this))
    VIRTUAL_METHOD(bool, isWeapon, 165, (), (this))
    VIRTUAL_METHOD(Entity*, getActiveWeapon, 267, (), (this))
    VIRTUAL_METHOD(ObsMode, getObserverMode, 293, (), (this))
    VIRTUAL_METHOD(Entity*, getObserverTarget, 294, (), (this))
    VIRTUAL_METHOD(WeaponInfo*, getWeaponInfo, 460, (), (this))

    auto getEyePosition() noexcept
    {
        Vector vec;
        VirtualMethod::call<void, 284>(this, std::ref(vec));
        return vec;
    }

    auto getAimPunch() noexcept
    {
        Vector vec;
        VirtualMethod::call<void, 345>(this, std::ref(vec));
        return vec;
    }

    bool canSee(Entity* other, const Vector& pos) noexcept;
    bool visibleTo(Entity* other) noexcept;
    [[nodiscard]] std::string getPlayerName(bool normalize) noexcept;

    PROP(hitboxSet, 0x9FC, int)                                                    // CBaseAnimating->m_nHitboxSet

    PROP(weaponId, 0x2FAA, WeaponId)                                               // CBaseAttributableItem->m_iItemDefinitionIndex

    PROP(clip, 0x3264, int)                                                        // CBaseCombatWeapon->m_iClip1
    PROP(isInReload, 0x3264 + 0x41, bool)                                          // CBaseCombatWeapon->m_iClip1 + 0x41
    PROP(reserveAmmoCount, 0x326C, int)                                            // CBaseCombatWeapon->m_iPrimaryReserveAmmoCount
    PROP(nextPrimaryAttack, 0x3238, float)                                         // CBaseCombatWeapon->m_flNextPrimaryAttack
        
    PROP(ownerEntity, 0x14C, int)                                                  // CBaseEntity->m_hOwnerEntity
    PROP(spotted, 0x93D, bool)                                                     // CBaseEntity->m_bSpotted
        
    PROP(inBombZone, 0x3954, bool)                                                 // CCSPlayer->m_bInBombZone
    PROP(flashDuration, 0xA40C - 0x8, float)                                       // CCSPlayer->m_flFlashMaxAlpha - 0x8
    PROP(shotsFired, 0xA380, int)                                                  // CCSPlayer->m_iShotsFired
        
    PROP(thrower, 0x29A0, int)                                                     // CBaseGrenade->m_hThrower
    PROP(grenadeExploded, 0x29E8, bool)
};
