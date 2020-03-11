#pragma once

#include "VirtualMethod.h"
#include "WeaponId.h"

struct WeaponInfo;

class WeaponSystem {
public:
    constexpr auto getWeaponInfo(WeaponId weaponId) noexcept
    {
        return VirtualMethod::call<WeaponInfo*, 2>(this, weaponId);
    }
};
