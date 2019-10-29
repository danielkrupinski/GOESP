#pragma once

#include "Utils.h"

struct Vector;

class DebugOverlay {
public:
    constexpr auto screenPosition(const Vector& point, Vector& screen) noexcept
    {
        return callVirtualMethod<bool, const Vector&, Vector&>(this, 13, point, screen);
    }
};
