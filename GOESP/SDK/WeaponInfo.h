#pragma once

#include <cstddef>

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
    Unknown
};

#define PAD(size) \
private: \
    std::byte _pad_##size[size]; \
public:

struct WeaponInfo {
    PAD(20)
    int maxClip;
    PAD(112)
    const char* name;
    PAD(60)
    WeaponType type;
    PAD(32)
    bool fullAuto;
    PAD(3)
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
