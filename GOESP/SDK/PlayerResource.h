#pragma once

#include "Entity.h"
#include "VirtualMethod.h"

class IPlayerResource {
public:
    VIRTUAL_METHOD_V(bool, isAlive, 5, (int index), (this, index))
    VIRTUAL_METHOD_V(const char*, getPlayerName, 8, (int index), (this, index))
    VIRTUAL_METHOD_V(int, getPlayerHealth, 14, (int index), (this, index))
};

class PlayerResource {
public:
    auto getIPlayerResource() noexcept
    {
        return reinterpret_cast<IPlayerResource*>(uintptr_t(this) + WIN32_UNIX(0x9D8, 0xF68));
    }

    PROP(competitiveRanking, WIN32_UNIX(0x1A84, 0x211C), int[65])
};
