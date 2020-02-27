#pragma once

#include "VirtualMethod.h"

#include <d3d9types.h>
#include <functional>
#include <tuple>

struct PlayerInfo {
    std::uint64_t version;
    union {
        std::uint64_t xuid;
        struct {
            std::uint32_t xuidLow;
            std::uint32_t xuidHigh;
        };
    };
    char name[128];
    int userId;
    char guid[33];
    std::uint32_t friendsId;
    char friendsName[128];
    bool fakeplayer;
    bool hltv;
    int customfiles[4];
    unsigned char filesdownloaded;
    int entityIndex;
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
