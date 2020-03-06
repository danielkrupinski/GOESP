#pragma once

#include <cstddef>

namespace VirtualMethod
{
    template <typename T, std::size_t Idx, typename ...Args>
    constexpr auto call(void* classBase, Args... args) noexcept
    {
        return ((*reinterpret_cast<T(__thiscall***)(void*, Args...)>(classBase))[Idx])(classBase, args...);
    }
}

#define VIRTUAL_METHOD_EX(name, returnType, idx, thisPtr) \
constexpr auto name() noexcept \
{ \
    return VirtualMethod::call<returnType, idx>(thisPtr); \
} \

#define VIRTUAL_METHOD(name, returnType, idx) \
VIRTUAL_METHOD_EX(name, returnType, idx, this)
