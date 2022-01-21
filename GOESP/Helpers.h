#pragma once

#include <cstdint>
#include <numbers>
#include <string>
#include <vector>

#include "imgui/imgui.h"

struct Color;

namespace Helpers
{
    unsigned int calculateColor(Color color, bool ignoreFlashbang = false) noexcept;
    unsigned int calculateColor(int r, int g, int b, int a) noexcept;
    void setAlphaFactor(float newAlphaFactor) noexcept;
    float getAlphaFactor() noexcept;
    void convertHSVtoRGB(float h, float s, float v, float& outR, float& outG, float& outB) noexcept;
    unsigned int healthColor(float fraction) noexcept;

    constexpr auto units2meters(float units) noexcept
    {
        return units * 0.0254f;
    }

    ImWchar* getFontGlyphRanges() noexcept;
    ImWchar* getFontGlyphRangesChinese() noexcept;

    constexpr auto deg2rad(float degrees) noexcept { return degrees * (std::numbers::pi_v<float> / 180.0f); }
    constexpr auto rad2deg(float radians) noexcept { return radians * (180.0f / std::numbers::pi_v<float>); }

    bool decodeVFONT(std::vector<char>& buffer) noexcept;
    std::vector<char> loadBinaryFile(const std::string& path) noexcept;

    template <typename T>
    constexpr auto relativeToAbsolute(std::uintptr_t address) noexcept
    {
        const auto offset = *reinterpret_cast<std::int32_t*>(address);
        const auto addressOfNextInstruction = address + 4;
        return (T)(addressOfNextInstruction + offset);
    }
}
