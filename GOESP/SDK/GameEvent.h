#pragma once

#include "VirtualMethod.h"

class GameEvent {
public:
    VIRTUAL_METHOD(const char*, getName, 1, (), (this))
    VIRTUAL_METHOD(int, getInt, 6, (const char* keyName, int defaultValue = 0), (this, keyName, defaultValue))
    VIRTUAL_METHOD(float, getFloat, 8, (const char* keyName, float defaultValue = 0.0f), (this, keyName, defaultValue))
    VIRTUAL_METHOD(const char*, getString, 9, (const char* keyName, const char* defaultValue = ""), (this, keyName, defaultValue))
};

class GameEventListener {
public:
    virtual ~GameEventListener() {}
    virtual void fireGameEvent(GameEvent* event) = 0;
    virtual int getEventDebugId() { return 42; }
};

class GameEventManager {
public:
    VIRTUAL_METHOD(void, addListener, 3, (GameEventListener* listener, const char* name, bool serverSide = false), (this, listener, name, serverSide))
    VIRTUAL_METHOD(void, removeListener, 5, (GameEventListener* listener), (this, listener))
};
