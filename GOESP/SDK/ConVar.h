#pragma once

#include "VirtualMethod.h"

struct ConVar {
    constexpr float getFloat() noexcept
    {
        return callVirtualMethod<float, 12>(this);
    }
};
