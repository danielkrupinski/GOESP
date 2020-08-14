#pragma once

#include "imgui/imgui.h"

struct Color;

namespace Helpers
{
    unsigned int calculateColor(Color color) noexcept;
    void setAlphaFactor(float newAlphaFactor) noexcept;

    constexpr auto units2meters(float units) noexcept
    {
        return units * 0.0254f;
    }

    ImWchar* getFontGlyphRanges() noexcept;
}