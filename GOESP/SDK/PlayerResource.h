#pragma once

#include "Entity.h"
#include "VirtualMethod.h"

struct Vector;

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

    PROP(bombsiteCenterA, WIN32_UNIX(0x1664, 0x1CFC), Vector)
    PROP(bombsiteCenterB, WIN32_UNIX(0x1670, 0x1D08), Vector)
    PROP(armor, WIN32_UNIX(0x187C, 0x1F14), int[65])
    PROP(competitiveRanking, WIN32_UNIX(0x1A84, 0x211C), int[65])
    PROP(competitiveWins, WIN32_UNIX(0x1B88, 0x2220), int[65])
};
