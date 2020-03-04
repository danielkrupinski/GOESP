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
public:
    VIRTUAL_METHOD(obbMins, const Vector&, 1)
    VIRTUAL_METHOD(obbMaxs, const Vector&, 2)
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
    VIRTUAL_METHOD_(getClientClass, ClientClass*, 2, this + 8)
    VIRTUAL_METHOD_(isDormant, bool, 9, this + 8)
    VIRTUAL_METHOD_(getModel, const Model*, 8, this + 4)

    VIRTUAL_METHOD(getCollideable, Collideable*, 3)
    VIRTUAL_METHOD(getAbsOrigin, Vector&, 10)
    VIRTUAL_METHOD(isAlive, bool, 155)
    VIRTUAL_METHOD(isWeapon, bool, 165)
    VIRTUAL_METHOD(getActiveWeapon, Entity*, 267)
    VIRTUAL_METHOD(getObserverMode, ObsMode, 293)
    VIRTUAL_METHOD(getObserverTarget, Entity*, 294)
    VIRTUAL_METHOD(getWeaponInfo, WeaponInfo*, 460)
   
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

    std::string getPlayerName(bool normalize) noexcept
    {
        std::string playerName = "unknown";

        PlayerInfo playerInfo;
        if (!interfaces->engine->getPlayerInfo(index(), playerInfo))
            return playerName;

        playerName = playerInfo.name;

        if (normalize) {
            if (wchar_t wide[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, 128, wide, 128)) {
                if (wchar_t wideNormalized[128]; NormalizeString(NormalizationKC, wide, -1, wideNormalized, 128)) {
                    if (char nameNormalized[128]; WideCharToMultiByte(CP_UTF8, 0, wideNormalized, -1, nameNormalized, 128, nullptr, nullptr))
                        playerName = nameNormalized;
                }
            }
        }

        playerName.erase(std::remove(playerName.begin(), playerName.end(), '\n'), playerName.end());
        return playerName;
    }

    NETVAR(weaponId, 0x2FAA, WeaponId)                                               // CBaseAttributableItem->m_iItemDefinitionIndex

    NETVAR(clip, 0x3254, int)                                                        // CBaseCombatWeapon->m_iClip1
    NETVAR(isInReload, 0x3254 + 0x41, bool)                                          // CBaseCombatWeapon->m_iClip1 + 0x41
    NETVAR(reserveAmmoCount, 0x325C, int)                                            // CBaseCombatWeapon->m_iPrimaryReserveAmmoCount
    NETVAR(nextPrimaryAttack, 0x3228, float)                                         // CBaseCombatWeapon->m_flNextPrimaryAttack

    NETVAR(index, 0x64, int)                                                         // CBaseEntity->m_bIsAutoaimTarget + 0x4
    NETVAR(ownerEntity, 0x14C, int)                                                  // CBaseEntity->m_hOwnerEntity

    NETVAR(health, 0x100, int)                                                       // CBasePlayer->m_iHealth
    NETVAR(flashDuration, 0xA40C - 0x8, float)                                       // CCSPlayer->m_flFlashMaxAlpha - 0x8

    NETVAR(coordinateFrame, 0x444, Matrix3x4)

    NETVAR(grenadeExploded, 0x29E8, bool)
};
