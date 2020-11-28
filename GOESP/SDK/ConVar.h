#pragma once

#include "VirtualMethod.h"

struct ConVar {
    VIRTUAL_METHOD(float, getFloat, (IS_WIN32() ? 12 : 15), (), (this))
};
