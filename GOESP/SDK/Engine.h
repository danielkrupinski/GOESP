#pragma once

#include "Utils.h"

class Engine {
public:
    constexpr auto getLocalPlayer() noexcept
    {
        return callVirtualMethod<int>(this, 12);
    }

    constexpr auto getMaxClients() noexcept
    {
        return callVirtualMethod<int>(this, 20);
    }

    constexpr auto isInGame() noexcept
    {
        return callVirtualMethod<bool>(this, 26);
    }
};