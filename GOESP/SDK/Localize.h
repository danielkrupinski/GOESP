#pragma once

#include "VirtualMethod.h"

class Localize {
public:
    constexpr auto find(const char* tokenName) noexcept
    {
        return callVirtualMethod<wchar_t*, 12>(this, tokenName);
    }
};
