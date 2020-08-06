#pragma once

#include "VirtualMethod.h"

class IPlayerResource {
public:
    VIRTUAL_METHOD(const char*, getPlayerName, 8, (int index), (this, index))
};

class PlayerResource {
public:
    auto getIPlayerResource() noexcept
    {
        return reinterpret_cast<IPlayerResource*>(uintptr_t(this) + 0x9D8);
    }
};
