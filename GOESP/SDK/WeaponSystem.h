#pragma once

#include "VirtualMethod.h"
#include "WeaponId.h"

struct WeaponInfo;

class WeaponSystem {
public:
    VIRTUAL_METHOD(WeaponInfo*, getWeaponInfo, 2, (WeaponId weaponId), (this, weaponId))
};
