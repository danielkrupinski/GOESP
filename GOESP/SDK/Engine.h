#pragma once

#include "VirtualMethod.h"

#include <Windows.h>
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

    VIRTUAL_METHOD(bool, getPlayerInfo, 8, (int entityIndex, PlayerInfo& playerInfo), (this, entityIndex, std::ref(playerInfo)))
    VIRTUAL_METHOD(int, getPlayerForUserId, 9, (int userId), (this, userId))
    VIRTUAL_METHOD(bool, isInGame, 26, (), (this))
    VIRTUAL_METHOD(const D3DMATRIX&, worldToScreenMatrix, 37, (), (this))
};
