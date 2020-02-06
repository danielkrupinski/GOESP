#pragma once

#include "VirtualMethod.h"

struct ConVar;

class Cvar {
public:
    constexpr auto findVar(const char* name) noexcept
    {
        return VirtualMethod::call<ConVar*, 15>(this, name);
    }
};
