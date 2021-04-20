#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "imgui/imgui.h"

#include "SDK/Entity.h"
#include "SDK/Matrix3x4.h"
#include "SDK/Vector.h"

struct LocalPlayerData;

struct PlayerData;
struct ObserverData;
struct WeaponData;
struct EntityData;
struct LootCrateData;
struct ProjectileData;
struct InfernoData;
struct BombData;
struct SmokeData;

namespace GameData
{
    void update() noexcept;
    void clearProjectileList() noexcept;
    void clearTextures() noexcept;
    void clearPlayersLastLocation() noexcept;
    void clearUnusedAvatars() noexcept;

    class Lock {
    private:
        static inline std::mutex mutex;
        std::scoped_lock<std::mutex> lock{ mutex };
    };

    // You have to acquire lock before using these getters
    bool worldToScreen(const Vector& in, ImVec2& out, bool floor = false) noexcept;
    [[nodiscard]] const LocalPlayerData& local() noexcept;
    [[nodiscard]] const std::vector<PlayerData>& players() noexcept;
    [[nodiscard]] const PlayerData* playerByHandle(int handle) noexcept;
    [[nodiscard]] const std::vector<ObserverData>& observers() noexcept;
    [[nodiscard]] const std::vector<WeaponData>& weapons() noexcept;
    [[nodiscard]] const std::vector<EntityData>& entities() noexcept;
    [[nodiscard]] const std::vector<LootCrateData>& lootCrates() noexcept;
    [[nodiscard]] const std::list<ProjectileData>& projectiles() noexcept;
    [[nodiscard]] const std::vector<InfernoData>& infernos() noexcept;
    [[nodiscard]] const std::vector<SmokeData>& smokes() noexcept;
    [[nodiscard]] const BombData& plantedC4() noexcept;
    [[nodiscard]] const std::string& gameMode() noexcept;
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
    int handle;
    float flashDuration;
    Vector aimPunch;
    Vector origin;
};

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
    float explosionTime = 0.0f;
    int handle;
    const char* name = nullptr;
    std::vector<std::pair<float, Vector>> trajectory;
};

struct PlayerData : BaseData {
    PlayerData(CSPlayer* entity) noexcept;
    PlayerData(const PlayerData&) = delete;
    PlayerData& operator=(const PlayerData&) = delete;
    PlayerData(PlayerData&& other) = default;
    PlayerData& operator=(PlayerData&& other) = default;

    void update(CSPlayer* entity) noexcept;
    const std::string& getRankName() const noexcept;
    ImTextureID getAvatarTexture() const noexcept;
    ImTextureID getRankTexture() const noexcept;
    float fadingAlpha() const noexcept;

    bool dormant;
    bool alive;
    bool inViewFrustum;
    bool enemy = false;
    bool visible = false;
    bool audible;
    bool spotted;
    bool immune;
    float lastContactTime = 0.0f;
    float flashDuration;
    int health;
    int armor;
    int userId;
    int handle;
    int money;
    int competitiveWins;
    Team team;
    std::uint64_t steamID;
    std::string name;
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
        ImTextureID get() const noexcept { return texture; }
    };
private:
    int skillgroup;
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
    ObserverData(CSPlayer* entity, CSPlayer* obs, bool targetIsLocalPlayer) noexcept;

    int playerUserId;
    int targetUserId;
    bool targetIsLocalPlayer;
};

struct BombData {
    void update() noexcept;

    float blowTime;
    float timerLength;
    int defuserHandle;
    float defuseCountDown;
    float defuseLength;
    int bombsite;
};

struct InfernoData {
    InfernoData(Entity* inferno) noexcept;

    std::vector<Vector> points;
};

struct SmokeData {
    SmokeData(const Vector& origin, int handle) noexcept;

    Vector origin;
    float explosionTime;
    int handle;
};
