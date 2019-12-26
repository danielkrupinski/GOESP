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

struct WeaponInfo {
private: std::byte pad_0[20];
public:
    int maxClip;
private: std::byte pad_1[112];
public:
    const char* name;
private: std::byte pad_2[60];
public:
    WeaponType type;
private: std::byte pad_3[32];
    bool fullAuto;
private: std::byte pad_4[3];
public:
    int damage;
    float armorRatio;
    int bullets;
    float penetration;
private: std::byte pad4[8];
public:
    float range;
    float rangeModifier;
private: std::byte pad5[16];
public:
    bool hasSilencer;
};
