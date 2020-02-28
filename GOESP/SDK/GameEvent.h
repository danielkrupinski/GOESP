#pragma once

#include "VirtualMethod.h"

class GameEvent {
public:
    constexpr auto getName() noexcept
    {
        return VirtualMethod::call<const char*, 1>(this);
    }

    constexpr auto getInt(const char* keyName, int defaultValue = 0) noexcept
    {
        return VirtualMethod::call<int, 6>(this, keyName, defaultValue);
    }

    constexpr auto getFloat(const char* keyName, float defaultValue = 0.0f) noexcept
    {
        return VirtualMethod::call<float, 8>(this, keyName, defaultValue);
    }

    constexpr auto getString(const char* keyName, const char* defaultValue = "") noexcept
    {
        return VirtualMethod::call<const char*, 9>(this, keyName, defaultValue);
    }
};

class GameEventListener {
public:
    virtual ~GameEventListener() {}
    virtual void fireGameEvent(GameEvent* event) = 0;
    virtual int getEventDebugId() { return 42; }
};

class GameEventManager {
public:
    constexpr auto addListener(GameEventListener* listener, const char* name) noexcept
    {
        return VirtualMethod::call<bool, 3>(this, listener, name, false);
    }

    constexpr auto removeListener(GameEventListener* listener) noexcept
    {
        VirtualMethod::call<void, 5>(this, listener);
    }
};
