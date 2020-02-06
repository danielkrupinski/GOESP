#pragma once

#include "VirtualMethod.h"

struct ConVar {
    constexpr float getFloat() noexcept
    {
        return VirtualMethod::call<float, 12>(this);
    }
};
