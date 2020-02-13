#pragma once

#include <tuple>

#include "imgui/imgui.h"
#include "ConfigStructs.h"

namespace Helpers
{
    constexpr auto rainbowColor(float time, float speed, float alpha) noexcept
    {
        return std::make_tuple(std::sin(speed * time) * 0.5f + 0.5f,
                               std::sin(speed * time + 2 * IM_PI / 3) * 0.5f + 0.5f,
                               std::sin(speed * time + 4 * IM_PI / 3) * 0.5f + 0.5f,
                               alpha);
    }

    constexpr auto calculateColor(const Color& color, float time) noexcept
    {
        if (color.rainbow)
            return ImGui::ColorConvertFloat4ToU32(rainbowColor(time, color.rainbowSpeed, color.color[3]));
        else
            return ImGui::ColorConvertFloat4ToU32(color.color);
    }

    constexpr auto units2meters(float units) noexcept
    {
        return units * 0.0254f;
    }
}