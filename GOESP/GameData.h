#pragma once

#include <list>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "SDK/Vector.h"

struct _LocalPlayerData;

struct _PlayerData;
struct _WeaponData;
struct _EntityData;
struct _LootCrateData;
struct _ProjectileData;

struct _D3DMATRIX;

namespace GameData
{
    void update() noexcept;
    void clearProjectileList() noexcept;

    class Lock {
    public:
        Lock() noexcept : lock{ mutex } {};
    private:
        std::scoped_lock<std::mutex> lock;
        static inline std::mutex mutex;
    };

    // You have to acquire lock before using these getters
    const _D3DMATRIX& toScreenMatrix() noexcept;
    const _LocalPlayerData& local() noexcept;
    const std::vector<_PlayerData>& players() noexcept;
    const std::vector<_WeaponData>& weapons() noexcept;
    const std::vector<_EntityData>& entities() noexcept;
    const std::vector<_LootCrateData>& lootCrates() noexcept;
    const std::list<_ProjectileData>& projectiles() noexcept;
}

struct _LocalPlayerData {
    void update() noexcept;

    bool exists = false;
    bool alive = false;
    bool inBombZone = false;
    bool inReload = false;
    bool shooting = false;
    float nextWeaponAttack = 0.0f;
    Vector aimPunch;
    Vector origin;
};

class Entity;

struct _BaseData {
    _BaseData(Entity* entity) noexcept;

    float distanceToLocal;
    Vector modelMins, modelMaxs;
    Vector obbMins, obbMaxs;
    Matrix3x4 coordinateFrame;
};

struct _EntityData final : _BaseData {
    _EntityData(Entity* entity) noexcept;
   
    const char* name;
};

struct _ProjectileData : _BaseData {
    _ProjectileData(Entity* projectile) noexcept;

    void update(Entity* projectile) noexcept;

    constexpr auto operator==(int otherHandle) const noexcept
    {
        return handle == otherHandle;
    }

    bool exploded = false;
    bool thrownByLocalPlayer = false;
    bool thrownByEnemy = false;
    int handle;
    const char* name = nullptr;
    std::vector<std::pair<float, Vector>> trajectory;
};

struct _PlayerData : _BaseData {
    _PlayerData(Entity* entity) noexcept;

    bool enemy = false;
    bool visible = false;
    bool audible;
    bool spotted;
    float flashDuration;
    std::string name;
    std::string activeWeapon;
    std::vector<std::pair<Vector, Vector>> bones;
};

struct _WeaponData : _BaseData {
    _WeaponData(Entity* entity) noexcept;

    int clip;
    int reserveAmmo;
    const char* group = "All";
    const char* name = "All";
    std::string displayName;
};

struct _LootCrateData : _BaseData {
    _LootCrateData(Entity* entity) noexcept;

    const char* name = nullptr;
};
