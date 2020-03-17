#pragma once

#include "VirtualMethod.h"

struct ConVar {
    VIRTUAL_METHOD(float, getFloat, 12, (), (this))
};
