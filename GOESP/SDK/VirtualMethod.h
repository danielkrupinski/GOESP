#pragma once

#include <cstddef>

#include "CallingConvention.h"

namespace VirtualMethod
{
    template <typename T, std::size_t Idx, typename ...Args>
    constexpr auto call(void* classBase, Args... args) noexcept
    {
        return (*reinterpret_cast<T(__THISCALL***)(void*, Args...)>(classBase))[Idx](classBase, args...);
    }
}

#define VIRTUAL_METHOD(returnType, name, idx, args, argsRaw) \
auto name args noexcept \
{ \
    return VirtualMethod::call<returnType, idx>argsRaw; \
}

#ifdef _WIN32

#define VIRTUAL_METHOD_V(returnType, name, idx, args, argsRaw) \
auto name args noexcept \
{ \
    return VirtualMethod::call<returnType, idx>argsRaw; \
}

#else

#define VIRTUAL_METHOD_V(returnType, name, idx, args, argsRaw) \
auto name args noexcept \
{ \
    return VirtualMethod::call<returnType, idx + 1>argsRaw; \
}

#endif
