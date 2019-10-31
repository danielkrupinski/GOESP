#pragma once

#include <cmath>
#include <tuple>

template<typename T, typename ...Args>
constexpr auto callVirtualMethod(void* classBase, int index, Args... args) noexcept
{
    return ((*reinterpret_cast<T(__thiscall***)(void*, Args...)>(classBase))[index])(classBase, args...);
}

constexpr unsigned packColor(float color[3]) noexcept
{
    return 0xFF000000 | unsigned(color[0] * 255) | (unsigned(color[1] * 255) << 8) | (unsigned(color[2] * 255) << 16);
}

constexpr auto rainbowColor(float time, float speed) noexcept
{
    return std::make_tuple(std::sin(speed * time) * 0.5f + 0.5f,
                           std::sin(speed * time + static_cast<float>(2 * M_PI / 3)) * 0.5f + 0.5f,
                           std::sin(speed * time + static_cast<float>(4 * M_PI / 3)) * 0.5f + 0.5f);
}
