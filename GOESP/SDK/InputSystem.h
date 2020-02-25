#pragma once

#include "VirtualMethod.h"

class InputSystem {
public:
    constexpr void enableInput(bool enable) noexcept
    {
        VirtualMethod::call<void, 11>(this, enable);
    }

    VIRTUAL_METHOD(resetInputState, void, 39)

    constexpr auto buttonCodeToString(int buttonCode) noexcept
    {
        return VirtualMethod::call<const char*, 40>(this, buttonCode);
    }

    constexpr auto stringTobuttonCode(const char* keyName) noexcept
    {
        return VirtualMethod::call<int, 42>(this, keyName);
    }
};
