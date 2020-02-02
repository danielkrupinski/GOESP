#pragma once
#include "Utils.h"

struct ConVar {
    constexpr float getFloat() noexcept
    {
        return callVirtualMethod<float, 12>(this);
    }
};
