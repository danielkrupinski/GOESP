#pragma once

#include "Pad.h"

enum class WeaponType {
    Knife = 0,
    Pistol,
    SubMachinegun,
    Rifle,
    Shotgun,
    SniperRifle,
    Machinegun,
    C4,
    Placeholder,
    Grenade,
    Unknown,
    StackableItem,
    Fists,
    BreachCharge,
    BumpMine,
    Tablet,
    Melee
};

struct WeaponInfo {
#ifdef _WIN32
    PAD(20)
#else
    PAD(32)
#endif
    int maxClip;
#ifdef _WIN32
    PAD(112)
#else
    PAD(204)
#endif
    const char* name;
#ifdef _WIN32
    PAD(60)
#else
    PAD(72)
#endif
    WeaponType type;
    PAD(4)
    int price;
    PAD(24)
    bool fullAuto;
#ifdef _WIN32
    PAD(3)
#else
    PAD(7)
#endif
    int damage;
    float armorRatio;
    int bullets;
    float penetration;
    PAD(8)
    float range;
    float rangeModifier;
    PAD(16)
    bool hasSilencer;
};
