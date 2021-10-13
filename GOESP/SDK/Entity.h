#pragma once

#include <string>

#include "Inconstructible.h"
#include "Vector.h"
#include "VirtualMethod.h"
#include "WeaponInfo.h"

struct ClientClass;
class Matrix3x4;
enum class WeaponId : short;
enum class WeaponType;

class Collideable {
public:
    INCONSTRUCTIBLE(Collideable)

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

enum class Team {
    None = 0,
    Spectators,
    TT,
    CT
};

constexpr auto EF_NODRAW = 0x20;

class CSPlayer;

class Entity {
public:
    INCONSTRUCTIBLE(Entity)

    VIRTUAL_METHOD(ClientClass*, getClientClass, 2, (), (this + sizeof(uintptr_t) * 2))
    VIRTUAL_METHOD(bool, isDormant, 9, (), (this + sizeof(uintptr_t) * 2))
    VIRTUAL_METHOD(int, index, 10, (), (this + sizeof(uintptr_t) * 2))

    VIRTUAL_METHOD(const Model*, getModel, 8, (), (this + sizeof(uintptr_t)))
    VIRTUAL_METHOD(bool, setupBones, 13, (Matrix3x4* out, int maxBones, int boneMask, float currentTime), (this + sizeof(uintptr_t), out, maxBones, boneMask, currentTime))
    VIRTUAL_METHOD(const Matrix3x4&, toWorldTransform, 32, (), (this + sizeof(uintptr_t)))

    VIRTUAL_METHOD_V(int&, handle, 2, (), (this))
    VIRTUAL_METHOD_V(Collideable*, getCollideable, 3, (), (this))

    VIRTUAL_METHOD(Vector&, getAbsOrigin, WIN32_UNIX(10, 12), (), (this))
    VIRTUAL_METHOD(Team, getTeamNumber, WIN32_UNIX(88, 128), (), (this))
    VIRTUAL_METHOD(int, getHealth, WIN32_UNIX(122, 167), (), (this))
    VIRTUAL_METHOD(bool, isAlive, WIN32_UNIX(156, 208), (), (this))
    VIRTUAL_METHOD(bool, isPlayer, WIN32_UNIX(158, 210), (), (this))
    VIRTUAL_METHOD(bool, isWeapon, WIN32_UNIX(166, 218), (), (this))
  
    VIRTUAL_METHOD(WeaponType, getWeaponType, WIN32_UNIX(455, 523), (), (this))
    VIRTUAL_METHOD(WeaponInfo*, getWeaponInfo, WIN32_UNIX(461, 529), (), (this))

    static CSPlayer* asPlayer(Entity* entity) noexcept
    {
        if (entity && entity->isPlayer())
            return reinterpret_cast<CSPlayer*>(entity);
        return nullptr;
    }

    auto isSniperRifle() noexcept
    {
        return getWeaponType() == WeaponType::SniperRifle;
    }

    auto isEffectActive(int effect) noexcept
    {
        return (effectFlags() & effect) != 0;
    }

    PROP(effectFlags, WIN32_UNIX(0xF0, 0x128), int)

    PROP(hitboxSet, WIN32_UNIX(0x9FC, 0xFA8), int)                                 // CBaseAnimating->m_nHitboxSet

    PROP(weaponId, WIN32_UNIX(0x2FBA, 0x37B2), WeaponId)                           // CBaseAttributableItem->m_iItemDefinitionIndex

    PROP(clip, WIN32_UNIX(0x3274, 0x3AE4), int)                                    // CBaseCombatWeapon->m_iClip1
    PROP(isInReload, WIN32_UNIX(0x32B5, 0x3B29), bool)                             // CBaseCombatWeapon->m_bInReload (client-side only)
    PROP(reserveAmmoCount, WIN32_UNIX(0x327C, 0x3AEC), int)                        // CBaseCombatWeapon->m_iPrimaryReserveAmmoCount
    PROP(nextPrimaryAttack, WIN32_UNIX(0x3248, 0x3AB8), float)                     // CBaseCombatWeapon->m_flNextPrimaryAttack

    PROP(prevOwner, WIN32_UNIX(0x3394, 0x3C1C), int)                               // CWeaponCSBase->m_hPrevOwner
        
    PROP(ownerEntity, WIN32_UNIX(0x14C, 0x184), int)                               // CBaseEntity->m_hOwnerEntity
    PROP(spotted, WIN32_UNIX(0x93D, 0xECD), bool)                                  // CBaseEntity->m_bSpotted
 
    PROP(thrower, WIN32_UNIX(0x29B0, 0x3040), int)                                 // CBaseGrenade->m_hThrower

    PROP(fireXDelta, WIN32_UNIX(0x9E4, 0xF80), int[100])                           // CInferno->m_fireXDelta
    PROP(fireYDelta, WIN32_UNIX(0xB74, 0x1110), int[100])                          // CInferno->m_fireYDelta
    PROP(fireZDelta, WIN32_UNIX(0xD04, 0x12A0), int[100])                          // CInferno->m_fireZDelta
    PROP(fireIsBurning, WIN32_UNIX(0xE94, 0x1430), bool[100])                      // CInferno->m_bFireIsBurning
    PROP(fireCount, WIN32_UNIX(0x13A8, 0x1944), int)                               // CInferno->m_fireCount

    PROP(mapHasBombTarget, WIN32_UNIX(0x71, 0x89), bool)                           // CCSGameRulesProxy->m_bMapHasBombTarget
};

class CSPlayer : public Entity {
public:
    VIRTUAL_METHOD(Entity*, getActiveWeapon, WIN32_UNIX(268, 331), (), (this))
    VIRTUAL_METHOD(ObsMode, getObserverMode, WIN32_UNIX(294, 357), (), (this))
    VIRTUAL_METHOD(Entity*, getObserverTarget, WIN32_UNIX(295, 358), (), (this))


#if IS_WIN32()
    auto getEyePosition() noexcept
    {
        Vector v;
        VirtualMethod::call<void, 285>(this, std::ref(v));
        return v;
    }

    auto getAimPunch() noexcept
    {
        Vector v;
        VirtualMethod::call<void, 346>(this, std::ref(v));
        return v;
    }
#else
    VIRTUAL_METHOD(Vector, getEyePosition, 348, (), (this))
    VIRTUAL_METHOD(Vector, getAimPunch, 409, (), (this))
#endif

    bool canSee(Entity* other, const Vector& pos) noexcept;
    bool visibleTo(CSPlayer* other) noexcept;
    [[nodiscard]] std::string getPlayerName() noexcept;
    void getPlayerName(char(&out)[128]) noexcept;
    int getUserId() noexcept;
    bool isEnemy() noexcept;
    bool isGOTV() noexcept;
    std::uint64_t getSteamID() noexcept;

    PROP(fov, WIN32_UNIX(0x31F4, 0x39A8), int)                                     // CBasePlayer->m_iFOV //0x31E4
    PROP(fovStart, WIN32_UNIX(0x31F8, 0x39AC), int)                                // CBasePlayer->m_iFOVStart //0x31E8
    PROP(defaultFov, WIN32_UNIX(0x333C, 0x3B14), int)                              // CBasePlayer->m_iDefaultFOV //0x332C
    PROP(lastPlaceName, WIN32_UNIX(0x35C4, 0x3DF0), char[18])                      // CBasePlayer->m_szLastPlaceName //0x35B4

    PROP(isScoped, WIN32_UNIX(0x9974, 0x4228), int)                               // CCSPlayer->m_bIsScoped //0x3928
    PROP(gunGameImmunity, WIN32_UNIX(0x9990, 0x4244), int)                        // CCSPlayer->m_bGunGameImmunity //0x3944
    PROP(flashDuration, WIN32_UNIX(0x1046C, 0xAD4C) - 0x8, float)                   // CCSPlayer->m_flFlashMaxAlpha - 0x8 //0xA41C
    PROP(shotsFired, WIN32_UNIX(0x103E0, 0xACC0), int)                              // CCSPlayer->m_iShotsFired //0xA390
    PROP(money, WIN32_UNIX(0x2FD8, 0xBCA8), int)                                   // CCSPlayer->m_iAccount //0xB364
};

class PlantedC4 : public Entity {
public:
    INCONSTRUCTIBLE(PlantedC4)

    PROP(ticking, WIN32_UNIX(0x2990, 0x3018), bool)                                // CPlantedC4->m_bBombTicking
    PROP(bombSite, WIN32_UNIX(0x2994, 0x301C), int)                                // CPlantedC4->m_nBombSite
    PROP(blowTime, WIN32_UNIX(0x29A0, 0x3028), float)                              // CPlantedC4->m_flC4Blow
    PROP(timerLength, WIN32_UNIX(0x29A4, 0x302C), float)                           // CPlantedC4->m_flTimerLength
    PROP(defuseLength, WIN32_UNIX(0x29B8, 0x3040), float)                          // CPlantedC4->m_flDefuseLength
    PROP(defuseCountDown, WIN32_UNIX(0x29BC, 0x3044), float)                       // CPlantedC4->m_flDefuseCountDown
    PROP(bombDefuser, WIN32_UNIX(0x29C4, 0x304C), int)                             // CPlantedC4->m_hBombDefuser
};
