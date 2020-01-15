#pragma once

#include <cmath>
#include <tuple>

#include "imgui/imgui.h"

namespace Helpers
{
    constexpr auto rainbowColor(float time, float speed, float alpha) noexcept
    {
        return std::make_tuple(std::sin(speed * time) * 0.5f + 0.5f,
                               std::sin(speed * time + static_cast<float>(2 * M_PI / 3)) * 0.5f + 0.5f,
                               std::sin(speed * time + static_cast<float>(4 * M_PI / 3)) * 0.5f + 0.5f,
                               alpha);
    }

    constexpr auto calculateColor(const std::array<float, 4>& color, bool rainbow, float rainbowSpeed, float time) noexcept
    {
        if (rainbow)
            return ImGui::ColorConvertFloat4ToU32(rainbowColor(time, rainbowSpeed, color[3]));
        else
            return ImGui::ColorConvertFloat4ToU32(color);
    }

    constexpr auto units2meters(float units) noexcept
    {
        return units * 0.0254f;
    }
}