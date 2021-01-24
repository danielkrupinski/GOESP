#pragma once

#include <cassert>

class CSPlayer;

class LocalPlayer {
public:
    void init(CSPlayer** entity) noexcept
    {
        assert(!localEntity);
        localEntity = entity;
    }

    constexpr operator bool() noexcept
    {
        assert(localEntity);
        return *localEntity != nullptr;
    }

    constexpr auto operator->() noexcept
    {
        assert(localEntity && *localEntity);
        return *localEntity;
    }

    constexpr auto get() noexcept
    {
        assert(localEntity && *localEntity);
        return *localEntity;
    }
private:
    CSPlayer** localEntity = nullptr;
};

inline LocalPlayer localPlayer;
