#pragma once

#include <cstddef>

namespace VirtualMethod
{
    template <typename T, std::size_t Idx, typename ...Args>
    constexpr auto call(void* classBase, Args... args) noexcept
    {
#ifdef _WIN32
        return ((*reinterpret_cast<T(__thiscall***)(void*, Args...)>(classBase))[Idx])(classBase, args...);
#else
        return ((*reinterpret_cast<T(***)(void*, Args...)>(classBase))[Idx])(classBase, args...);
#endif
    }
}

#define VIRTUAL_METHOD(returnType, name, idx, args, argsRaw) \
auto name args noexcept \
{ \
    return VirtualMethod::call<returnType, idx>argsRaw; \
}
