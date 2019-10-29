#pragma once

#include "Utils.h"

#include <tuple>

class Engine {
public:
    constexpr auto getScreenSize() noexcept
    {
        int w = 0, h = 0;
        callVirtualMethod<void, int&, int&>(this, 5, w, h);
        return std::make_pair(w, h);
    }

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