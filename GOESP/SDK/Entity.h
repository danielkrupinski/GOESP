#pragma once

#include <string>

#include "Vector.h"
#include "VirtualMethod.h"
#include "WeaponInfo.h"

struct ClientClass;
class Matrix3x4;
enum class WeaponId : short;
struct WeaponInfo;
enum class WeaponType;

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
[[nodiscard]] std::add_lvalue_reference_t<std::add_const_t<type>> func_name() noexcept \
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
    VIRTUAL_METHOD(ClientClass*, getClientClass, 2, (), (this + sizeof(uintptr_t) * 2))
    VIRTUAL_METHOD(bool, isDormant, 9, (), (this + sizeof(uintptr_t) * 2))
    VIRTUAL_METHOD(int, index, 10, (), (this + sizeof(uintptr_t) * 2))

    VIRTUAL_METHOD(const Model*, getModel, 8, (), (this + sizeof(uintptr_t)))
    VIRTUAL_METHOD(bool, setupBones, 13, (Matrix3x4* out, int maxBones, int boneMask, float currentTime), (this + sizeof(uintptr_t), out, maxBones, boneMask, currentTime))
    VIRTUAL_METHOD(const Matrix3x4&, toWorldTransform, 32, (), (this + sizeof(uintptr_t)))

    VIRTUAL_METHOD(int&, handle, 2, (), (this))
    VIRTUAL_METHOD(Collideable*, getCollideable, 3, (), (this))

#ifdef _WIN32
    VIRTUAL_METHOD(Vector&, getAbsOrigin, 10, (), (this))
    VIRTUAL_METHOD(int, getHealth, 121, (), (this))
    VIRTUAL_METHOD(bool, isAlive, 155, (), (this))
    VIRTUAL_METHOD(bool, isPlayer, 157, (), (this))
    VIRTUAL_METHOD(bool, isWeapon, 165, (), (this))
    VIRTUAL_METHOD(Entity*, getActiveWeapon, 267, (), (this))
    VIRTUAL_METHOD(void, getEyePosition, 284, (Vector& v), (this, std::ref(v)))
    VIRTUAL_METHOD(ObsMode, getObserverMode, 293, (), (this))
    VIRTUAL_METHOD(Entity*, getObserverTarget, 294, (), (this))
    VIRTUAL_METHOD(void, getAimPunch, 345, (Vector& v), (this, std::ref(v)))
    VIRTUAL_METHOD(WeaponType, getWeaponType, 454, (), (this))
    VIRTUAL_METHOD(WeaponInfo*, getWeaponInfo, 460, (), (this))
#elif __linux__
    VIRTUAL_METHOD(Vector&, getAbsOrigin, 12, (), (this))
    VIRTUAL_METHOD(int, getHealth, 166, (), (this))
    VIRTUAL_METHOD(bool, isAlive, 207, (), (this))
    VIRTUAL_METHOD(bool, isPlayer, 209, (), (this))
    VIRTUAL_METHOD(bool, isWeapon, 217, (), (this))
    VIRTUAL_METHOD(Entity*, getActiveWeapon, 330, (), (this))
    VIRTUAL_METHOD(void, getEyePosition, 347, (Vector& v), (this, std::ref(v)))
    VIRTUAL_METHOD(ObsMode, getObserverMode, 356, (), (this))
    VIRTUAL_METHOD(Entity*, getObserverTarget, 357, (), (this))
    VIRTUAL_METHOD(void, getAimPunch, 408, (Vector& v), (this, std::ref(v)))
    VIRTUAL_METHOD(WeaponType, getWeaponType, 522, (), (this))
    VIRTUAL_METHOD(WeaponInfo*, getWeaponInfo, 528, (), (this))
#endif
        
    auto isSniperRifle() noexcept
    {
        return getWeaponType() == WeaponType::SniperRifle;
    }

    auto getEyePosition() noexcept
    {
        Vector vec;
        getEyePosition(vec);
        return vec;
    }

    auto getAimPunch() noexcept
    {
        Vector vec;
        getAimPunch(vec);
        return vec;
    }

    bool canSee(Entity* other, const Vector& pos) noexcept;
    bool visibleTo(Entity* other) noexcept;
    [[nodiscard]] std::string getPlayerName() noexcept;
    void getPlayerName(char(&out)[128]) noexcept;

#ifdef _WIN32
    PROP(hitboxSet, 0x9FC, int)                                                    // CBaseAnimating->m_nHitboxSet

    PROP(weaponId, 0x2FAA, WeaponId)                                               // CBaseAttributableItem->m_iItemDefinitionIndex

    PROP(clip, 0x3264, int)                                                        // CBaseCombatWeapon->m_iClip1
    PROP(isInReload, 0x32A5, bool)                                                 // CBaseCombatWeapon->m_bInReload (client-side only)
    PROP(reserveAmmoCount, 0x326C, int)                                            // CBaseCombatWeapon->m_iPrimaryReserveAmmoCount
    PROP(nextPrimaryAttack, 0x3238, float)                                         // CBaseCombatWeapon->m_flNextPrimaryAttack

    PROP(prevOwner, 0x3384, int)                                                   // CWeaponCSBase->m_hPrevOwner
        
    PROP(ownerEntity, 0x14C, int)                                                  // CBaseEntity->m_hOwnerEntity
    PROP(spotted, 0x93D, bool)                                                     // CBaseEntity->m_bSpotted
    
    PROP(fov, 0x31E4, int)                                                         // CBasePlayer->m_iFOV
    PROP(fovStart, 0x31E8, int)                                                    // CBasePlayer->m_iFOVStart
    PROP(defaultFov, 0x332C, int)                                                  // CBasePlayer->m_iDefaultFOV
 
    PROP(isScoped, 0x3928, bool)                                                   // CCSPlayer->m_bIsScoped
    PROP(gunGameImmunity, 0x3944, bool)                                            // CCSPlayer->m_bGunGameImmunity
    PROP(flashDuration, 0xA41C - 0x8, float)                                       // CCSPlayer->m_flFlashMaxAlpha - 0x8
    PROP(shotsFired, 0xA390, int)                                                  // CCSPlayer->m_iShotsFired
        
    PROP(thrower, 0x29A0, int)                                                     // CBaseGrenade->m_hThrower
    PROP(grenadeExploded, 0x29E8, bool)

    PROP(bombTicking, 0x2980, bool)                                                // CPlantedC4->m_bBombTicking
    PROP(bombSite, 0x2984, bool)                                                   // CPlantedC4->m_nBombSite

#elif __linux__
    PROP(hitboxSet, 0xFA8, int)                                                    // CBaseAnimating->m_nHitboxSet

    PROP(weaponId, 0x37b2, WeaponId)                                               // CBaseAttributableItem->m_iItemDefinitionIndex

    PROP(clip, 0x3AE4, int)                                                        // CBaseCombatWeapon->m_iClip1
    PROP(isInReload, 0x3B29, bool)                                                 // CBaseCombatWeapon->m_bInReload (client-side only)

    PROP(reserveAmmoCount, 0x3AEC, int)                                            // CBaseCombatWeapon->m_iPrimaryReserveAmmoCount
    PROP(nextPrimaryAttack, 0x3AB8, float)                                         // CBaseCombatWeapon->m_flNextPrimaryAttack

    PROP(prevOwner, 0x3C1C, int)                                                   // CWeaponCSBase->m_hPrevOwner

    PROP(ownerEntity, 0x184, int)                                                  // CBaseEntity->m_hOwnerEntity
    PROP(spotted, 0xECD, bool)                                                     // CBaseEntity->m_bSpotted

    PROP(fov, 0x39A8, int)                                                         // CBasePlayer->m_iFOV
    PROP(fovStart, 0x39AC, int)                                                    // CBasePlayer->m_iFOVStart
    PROP(defaultFov, 0x3B14, int)                                                  // CBasePlayer->m_iDefaultFOV

    PROP(isScoped, 0x4228, bool)                                                   // CCSPlayer->m_bIsScoped
    PROP(gunGameImmunity, 0x4244, bool)                                            // CCSPlayer->m_bGunGameImmunity
    PROP(flashDuration, 0xAD4C - 0x8, float)                                       // CCSPlayer->m_flFlashMaxAlpha - 0x8
    PROP(shotsFired, 0xACC0, int)                                                  // CCSPlayer->m_iShotsFired

    PROP(thrower, 0x3040, int)                                                     // CBaseGrenade->m_hThrower

    // TODO: update this
    // PROP(grenadeExploded, 0x29E8, bool)
    bool grenadeExploded()
    {
        return false;
    }

#endif
};

