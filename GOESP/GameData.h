#pragma once

#include <list>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "imgui/imgui.h"

#include "SDK/Entity.h"
#include "SDK/Vector.h"

struct LocalPlayerData;

struct PlayerData;
struct ObserverData;
struct WeaponData;
struct EntityData;
struct LootCrateData;
struct ProjectileData;

struct Matrix4x4;

namespace GameData
{
    void update() noexcept;
    void clearProjectileList() noexcept;
    void clearTextures() noexcept;

    class Lock {
    public:
        Lock() noexcept : lock{ mutex } {}
    private:
        std::scoped_lock<std::mutex> lock;
        static inline std::mutex mutex;
    };

    // You have to acquire lock before using these getters
    const Matrix4x4& toScreenMatrix() noexcept;
    const LocalPlayerData& local() noexcept;
    const std::vector<PlayerData>& players() noexcept;
    const std::vector<ObserverData>& observers() noexcept;
    const std::vector<WeaponData>& weapons() noexcept;
    const std::vector<EntityData>& entities() noexcept;
    const std::vector<LootCrateData>& lootCrates() noexcept;
    const std::list<ProjectileData>& projectiles() noexcept;
}

struct LocalPlayerData {
    void update() noexcept;

    bool exists = false;
    bool alive = false;
    bool inReload = false;
    bool shooting = false;
    bool noScope = false;
    float nextWeaponAttack = 0.0f;
    int fov;
    float flashDuration;
    Vector aimPunch;
    Vector origin;
};

class Entity;

struct BaseData {
    BaseData(Entity* entity) noexcept;

    constexpr auto operator<(const BaseData& other) const
    {
        return distanceToLocal > other.distanceToLocal;
    }

    float distanceToLocal;
    Vector obbMins, obbMaxs;
    Matrix3x4 coordinateFrame;
};

struct EntityData final : BaseData {
    EntityData(Entity* entity) noexcept;

    const char* name;
};

struct ProjectileData : BaseData {
    ProjectileData(Entity* projectile) noexcept;
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

struct PlayerData : BaseData {
    PlayerData(Entity* entity) noexcept;
    PlayerData(const PlayerData&) = delete;
    PlayerData& operator=(const PlayerData&) = delete;
    PlayerData(PlayerData&& other) = default;
    PlayerData& operator=(PlayerData&& other) = default;

    void update(Entity* entity) noexcept;
    ImTextureID getAvatarTexture() const noexcept;
    ImTextureID getRankTexture() const noexcept;
    void clearAvatarTexture() noexcept { avatarTexture = {}; }

    bool dormant;
    bool alive;
    bool inViewFrustum;
    bool enemy = false;
    bool visible = false;
    bool audible;
    bool spotted;
    bool immune;
    bool hasAvatar = false;
    float fadingEndTime = 0.0f;
    float flashDuration;
    int health;
    int userId;
    int handle;
    int money;
    Team team;
    std::uint64_t steamID;
    char name[128];
    std::string activeWeapon;
    Vector origin;
    std::vector<std::pair<Vector, Vector>> bones;
    Vector headMins, headMaxs;
    std::string lastPlaceName;

    class Texture {
        ImTextureID texture = nullptr;
    public:
        Texture() = default;
        ~Texture();
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&& other) noexcept : texture{ other.texture } { other.texture = nullptr; }
        Texture& operator=(Texture&& other) noexcept { clear(); texture = other.texture; other.texture = nullptr; return *this; }

        void init(int width, int height, const std::uint8_t* data) noexcept;
        void clear() noexcept;
        ImTextureID get() noexcept { return texture; }
    };
private:
    int skillgroup;

    mutable Texture avatarTexture;
    std::uint8_t avatarRGBA[4 * 32 * 32 * sizeof(char)];
};

struct WeaponData : BaseData {
    WeaponData(Entity* entity) noexcept;

    int clip;
    int reserveAmmo;
    const char* group = "All";
    const char* name = "All";
    std::string displayName;
};

struct LootCrateData : BaseData {
    LootCrateData(Entity* entity) noexcept;

    const char* name = nullptr;
};

struct ObserverData {
    ObserverData(Entity* entity, Entity* obs, bool targetIsLocalPlayer) noexcept;

    int playerUserId;
    int targetUserId;
    bool targetIsLocalPlayer;
};

struct BombData {
    BombData(Entity* entity) noexcept;

    int bombsite;
    float blowTime;
    float defuseCountDown;
};
