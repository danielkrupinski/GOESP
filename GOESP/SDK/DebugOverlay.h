#pragma once

#include "../imgui/imgui.h"
#include "Utils.h"
#include "Vector.h"

class DebugOverlay {
public:
    constexpr auto screenPosition(const Vector& point, ImVec2& screen) noexcept
    {
        Vector out{ };
        auto result = callVirtualMethod<bool, const Vector&, Vector&>(this, 13, point, out);
        screen.x = out.x;
        screen.y = out.y;
        return result;
    }
};
