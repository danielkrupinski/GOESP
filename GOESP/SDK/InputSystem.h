#pragma once

#include "Utils.h"

class InputSystem {
public:
    constexpr void enableInput(bool enable) noexcept
    {
        callVirtualMethod<void, 11>(this, enable);
    }
};
