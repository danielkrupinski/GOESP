#pragma once

#include "VirtualMethod.h"

class Localize {
public:
    constexpr auto find(const char* tokenName) noexcept
    {
        return VirtualMethod::call<const wchar_t*, 12>(this, tokenName);
    }
};
