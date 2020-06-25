#pragma once

#include "VirtualMethod.h"

class InputSystem {
public:
    VIRTUAL_METHOD(void, enableInput, 11, (bool enable), (this, enable))
    VIRTUAL_METHOD(void, resetInputState, 39, (), (this))
    VIRTUAL_METHOD(const char*, buttonCodeToString, 40, (int buttonCode), (this, buttonCode))
    VIRTUAL_METHOD(int, stringToButtonCode, 42, (const char* keyName), (this, keyName))

    // does not support mouse buttons
    VIRTUAL_METHOD(int, virtualKeyToButtonCode, 45, (int virtualKey), (this, virtualKey))
    VIRTUAL_METHOD(int, buttonCodeToVirtualKey, 46, (int buttonCode), (this, buttonCode))

    auto keyToButtonCode(int key) noexcept
    {
        if (key < 512)
            return virtualKeyToButtonCode(key);

        if (key >= 512 && key <= 518)
            return key - 405;

        return 0;
    }

    auto buttonCodeToKey(int buttonCode) noexcept
    {
        if (buttonCode < 107)
            return buttonCodeToVirtualKey(buttonCode);

        if (buttonCode >= 107 && buttonCode <= 113)
            return buttonCode + 405;

        return 0;
    }

    auto keyToString(int key) noexcept
    {
        return buttonCodeToString(keyToButtonCode(key));
    }

    auto stringToKey(const char* keyName) noexcept
    {
        return buttonCodeToKey(stringToButtonCode(keyName));
    }
};
