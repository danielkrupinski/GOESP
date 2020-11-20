#include <cmath>
#include <fstream>
#include <numbers>
#include <tuple>

#include "imgui/imgui.h"

#include "ConfigStructs.h"
#include "GameData.h"
#include "Helpers.h"
#include "Memory.h"
#include "SDK/GlobalVars.h"
#include "Hacks/Misc.h"

static auto rainbowColor(float time, float speed, float alpha) noexcept
{
    constexpr float pi = std::numbers::pi_v<float>;
    return std::array{ std::sin(speed * time) * 0.5f + 0.5f,
                       std::sin(speed * time + 2 * pi / 3) * 0.5f + 0.5f,
                       std::sin(speed * time + 4 * pi / 3) * 0.5f + 0.5f,
                       alpha };
}

static float alphaFactor = 1.0f;

unsigned int Helpers::calculateColor(Color color) noexcept
{
    color.color[3] *= alphaFactor;

    if (!Misc::ignoresFlashbang())
        color.color[3] -= color.color[3] * GameData::local().flashDuration / 255.0f;
    return ImGui::ColorConvertFloat4ToU32(color.rainbow ? rainbowColor(memory->globalVars->realtime, color.rainbowSpeed, color.color[3]) : color.color);
}

unsigned int Helpers::calculateColor(int r, int g, int b, int a) noexcept
{
    if (!Misc::ignoresFlashbang())
        a -= static_cast<int>(a * GameData::local().flashDuration / 255.0f);
    return IM_COL32(r, g, b, a * alphaFactor);
}

void Helpers::setAlphaFactor(float newAlphaFactor) noexcept
{
    alphaFactor = newAlphaFactor;
}

float Helpers::getAlphaFactor() noexcept
{
    return alphaFactor;
}

float Helpers::fadingAlpha(float endTime) noexcept
{
    constexpr float fadeTime = 1.75f;
    return std::clamp(std::max(endTime - memory->globalVars->realtime, 0.0f) / fadeTime, 0.0f, 1.0f);
}

ImWchar* Helpers::getFontGlyphRanges() noexcept
{
    static ImVector<ImWchar> ranges;
    if (ranges.empty()) {
        ImFontGlyphRangesBuilder builder;
        constexpr ImWchar baseRanges[]{ 0x0100, 0x024F, 0x0370, 0x03FF, 0x0600, 0x06FF, 0x0E00, 0x0E7F, 0 };
        builder.AddRanges(baseRanges);
        builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
        builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
        builder.AddText("\u9F8D\u738B\u2122\u1ED3\u1EF1\u1EA1\u1ED5");
        builder.BuildRanges(&ranges);
    }
    return ranges.Data;
}

ImWchar* Helpers::getFontGlyphRangesChinese() noexcept
{
    static ImVector<ImWchar> ranges;
    if (ranges.empty()) {
        ImFontGlyphRangesBuilder builder;
        builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
        builder.AddChar(0x739B); builder.AddChar(0x5C14); builder.AddChar(0x6D1B); builder.AddChar(0x97E6);
        builder.AddChar(0x76D4); builder.AddChar(0x64BC); builder.AddChar(0x9975); builder.AddChar(0x8166);
        builder.AddChar(0x96FB); builder.AddChar(0x7DAD); builder.AddChar(0x5F48); builder.AddChar(0x723E);
        builder.AddChar(0x85A9); builder.AddChar(0x8A98); builder.AddChar(0x990C); builder.AddChar(0x51F1);
        builder.AddChar(0x9727); builder.AddChar(0x9583); builder.AddChar(0x7159); builder.AddChar(0x6771);
        builder.AddChar(0x6C40);
        builder.BuildRanges(&ranges);
    }
    return ranges.Data;
}

bool Helpers::decodeVFONT(std::vector<char>& buffer) noexcept
{
    constexpr std::string_view tag = "VFONT1";
    unsigned char magic = 0xA7;

    if (buffer.size() <= tag.length())
        return false;

    const auto tagIndex = buffer.size() - tag.length();
    if (std::memcmp(tag.data(), &buffer[tagIndex], tag.length()))
        return false;

    unsigned char saltBytes = buffer[tagIndex - 1];
    const auto saltIndex = tagIndex - saltBytes;
    --saltBytes;

    for (std::size_t i = 0; i < saltBytes; ++i)
        magic ^= (buffer[saltIndex + i] + 0xA7) % 0x100;

    for (std::size_t i = 0; i < saltIndex; ++i) {
        unsigned char xored = buffer[i] ^ magic;
        magic = (buffer[i] + 0xA7) % 0x100;
        buffer[i] = xored;
    }

    buffer.resize(saltIndex);
    return true;
}

std::vector<char> Helpers::loadBinaryFile(const std::string& path) noexcept
{
    std::vector<char> result;
    std::ifstream in{ path, std::ios::binary };
    if (!in)
        return result;
    in.seekg(0, in.end);
    result.resize(static_cast<std::size_t>(in.tellg()));
    in.seekg(0, in.beg);
    in.read(result.data(), result.size());
    return result;
}
