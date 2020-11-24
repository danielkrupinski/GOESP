#pragma once

#include "VirtualMethod.h"

struct ConVar {
#ifdef _WIN32
    VIRTUAL_METHOD(float, getFloat, 12, (), (this))
#else
    VIRTUAL_METHOD(float, getFloat, 15, (), (this))
#endif
};
