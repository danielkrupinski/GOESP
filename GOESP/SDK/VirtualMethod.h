#pragma once

#include <cstddef>

template<typename T, std::size_t Index, typename ...Args>
constexpr auto callVirtualMethod(void* classBase, Args... args) noexcept
{
    return ((*reinterpret_cast<T(__thiscall***)(void*, Args...)>(classBase))[Index])(classBase, args...);
}
