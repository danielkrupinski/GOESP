#pragma once

struct Color;

namespace Helpers
{
    unsigned int calculateColor(Color color) noexcept;

    constexpr auto units2meters(float units) noexcept
    {
        return units * 0.0254f;
    }
}