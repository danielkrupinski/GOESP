#pragma once

#include "VirtualMethod.h"

class InputSystem {
public:
    constexpr void enableInput(bool enable) noexcept
    {
        VirtualMethod::call<void, 11>(this, enable);
    }

    VIRTUAL_METHOD_V2(void, resetInputState, 39, (), (this))

    constexpr auto buttonCodeToString(int buttonCode) noexcept
    {
        return VirtualMethod::call<const char*, 40>(this, buttonCode);
    }

    constexpr auto stringToButtonCode(const char* keyName) noexcept
    {
        return VirtualMethod::call<int, 42>(this, keyName);
    }

    // does not support mouse buttons
    constexpr auto virtualKeyToButtonCode(int virtualKey) noexcept
    {
        return VirtualMethod::call<int, 45>(this, virtualKey);
    }

    // does not support mouse buttons
    constexpr auto buttonCodeToVirtualKey(int buttonCode) noexcept
    {
        return VirtualMethod::call<int, 46>(this, buttonCode);
    }

    constexpr auto keyToButtonCode(int key) noexcept
    {
        if (key < 512)
            return virtualKeyToButtonCode(key);

        if (key >= 512 && key <= 518)
            return key - 405;

        return 0;
    }

    constexpr auto buttonCodeToKey(int buttonCode) noexcept
    {
        if (buttonCode < 107)
            return buttonCodeToVirtualKey(buttonCode);

        if (buttonCode >= 107 && buttonCode <= 113)
            return buttonCode + 405;

        return 0;
    }

    constexpr auto keyToString(int key) noexcept
    {
        return buttonCodeToString(keyToButtonCode(key));
    }

    constexpr auto stringToKey(const char* keyName) noexcept
    {
        return buttonCodeToKey(stringToButtonCode(keyName));
    }
};
