#pragma once

#include "VirtualMethod.h"

#include <d3d9types.h>
#include <functional>
#include <tuple>

struct PlayerInfo {
    std::int64_t pad;
    union {
        std::int64_t xuid;
        struct {
            std::int32_t xuidLow;
            std::int32_t xuidHigh;
        };
    };
    char name[128];
    int userId;
    char steamIdString[20];
    char pad1[16];
    unsigned long steamId;
    char friendsName[128];
    bool fakeplayer;
    bool ishltv;
    unsigned int customfiles[4];
    unsigned char filesdownloaded;
};

class Engine {
public:
    auto getScreenSize() noexcept
    {
        int w, h;
        VirtualMethod::call<void, 5>(this, std::ref(w), std::ref(h));
        return std::make_pair(w, h);
    }

    constexpr auto getPlayerInfo(int entityIndex, PlayerInfo& playerInfo) noexcept
    {
        return VirtualMethod::call<bool, 8>(this, entityIndex, std::ref(playerInfo));
    }

    constexpr auto getPlayerForUserId(int userId) noexcept
    {
        return VirtualMethod::call<int, 9>(this, userId);
    }

    VIRTUAL_METHOD(getLocalPlayer, int, 12)
    VIRTUAL_METHOD(isInGame, bool, 26)
    VIRTUAL_METHOD(worldToScreenMatrix, const D3DMATRIX&, 37)
};
