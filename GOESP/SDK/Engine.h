#pragma once

#include "VirtualMethod.h"

#include <d3d9types.h>
#include <functional>
#include <tuple>

struct PlayerInfo {
    __int64 pad;
    union {
        __int64 steamID64;
        __int32 xuidLow;
        __int32 xuidHigh;
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
    constexpr auto getScreenSize() noexcept
    {
        int w = 0, h = 0;
        VirtualMethod::call<void, 5>(this, std::ref(w), std::ref(h));
        return std::make_pair(w, h);
    }

    constexpr auto getPlayerInfo(int entityIndex, PlayerInfo& playerInfo) noexcept
    {
        return VirtualMethod::call<bool, 8>(this, entityIndex, std::ref(playerInfo));
    }

    constexpr auto getLocalPlayer() noexcept
    {
        return VirtualMethod::call<int, 12>(this);
    }

    constexpr auto getMaxClients() noexcept
    {
        return VirtualMethod::call<int, 20>(this);
    }

    constexpr auto isInGame() noexcept
    {
        return VirtualMethod::call<bool, 26>(this);
    }

    constexpr auto worldToScreenMatrix() noexcept
    {
        return VirtualMethod::call<const D3DMATRIX&, 37>(this);
    }
};
